/*
Copyright (C) 2020-2024 popkc(popkc at 163 dot com)
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "core/fileinfo.h"
#include "core/widgetoutput.h"
#include "dialog/dialogconfig.h"
#include "mainwindow.h"
#include "pch.h"
#include "ui_mainwindow.h"

FileInfo::FileInfo(QObject *parent)
    : QObject(parent)
{
    content = nullptr;
    pieceLoaded = nullptr;
    sliderShift = 0;
}

FileInfo::~FileInfo()
{
    if (content)
        delete[] content;

    if (pieceLoaded)
        delete[] pieceLoaded;
}

void FileInfo::loadFile(const QString &fn)
{
    close();
    file.setFileName(fn);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(w, tr("错误"), tr("打开文件失败，文件名：") + fn);
        return;
    }

#if QT_POINTER_SIZE == 4
    if (file.size() > 2147483647) {
        QMessageBox::warning(w, tr("错误"), tr("文件大小不能超过2GB。"));
        return;
    }
#endif

    w->currentOutput->offset = 0;

    if (w->recentFiles.empty() || w->recentFiles[0] != fn) {
        w->recentFiles.prepend(fn);
        w->recentFiles.removeDuplicates();
        while (w->recentFiles.size() > 20)
            w->recentFiles.removeLast();
        w->settings->setValue("?app/recentfiles", w->recentFiles);
        w->resetRecentFiles();
    }

    if (pieceLoaded)
        delete[] pieceLoaded;
    pieceLoadedSize = (file.size() - 1) / PIECESIZE + 1;
    pieceLoaded = new std::atomic<bool>[pieceLoadedSize];
#if (__cplusplus < 202000)
    memset(pieceLoaded, 0, sizeof(std::atomic<bool>) * pieceLoadedSize);
#endif
    if (file.size() != 0) {
        try {
            content = new char[file.size()];
        } catch (const std::exception &) {
            QMessageBox::warning(w, tr("错误"), tr("没有足够的内存打开此文件。"));
            file.close();
            w->currentOutput->needRedraw = true;
            w->currentOutput->update();
            return;
        }
        contentEnd = content + file.size();
        uint64_t m = file.size() - 1;
        sliderShift = 0;
        while (m > INT32_MAX) {
            sliderShift++;
            m >>= 1;
        }
        w->ui->verticalScrollBar->setMaximum(m);
    }
    else {
        w->ui->verticalScrollBar->setMaximum(0);
        sliderShift = 0;
    }

    QByteArray ba = w->settings->value(fn + "/?codec").toByteArray();
    if (ba.isEmpty())
        detectCodec();
    else {
        bool f = false;
        auto alist = w->ui->menuCodec->actions();
        for (auto a : alist) {
            if (a->isCheckable()) {
                if (a->iconText().toLocal8Bit() == ba) {
                    w->codecTriggered(a);
                    f = true;
                    break;
                }
            }
        }

        if (!f)
            codec = QTextCodec::codecForName(ba);
    }

    currentPos = w->settings->value(fn + "/?pos", 0).toUInt();
    if ((uintptr_t)file.size() <= currentPos)
        currentPos = 0;
    w->ui->verticalScrollBar->setValue(currentPos >> sliderShift);

    w->currentOutput->needRedraw = true;
    w->currentOutput->update();
    w->ui->menuJump->setEnabled(true);
    w->ui->menuAutoRead->setEnabled(true);

    renewMapIndex();
    setTitle();
}

void FileInfo::close()
{
    if (!file.isOpen())
        return;

    saveReadPos();
    file.close();
    if (content) {
        delete[] content;
        content = nullptr;
    }

    if (w->ui->actionRead->isChecked()) {
        w->ui->actionRead->setChecked(false);
    }
    if (w->ui->actionAutoRoll->isChecked()) {
        w->ui->actionAutoRoll->setChecked(false);
    }
    w->setWindowTitle(POPKC_TITLE);
    w->ui->menuJump->setEnabled(false);
    w->ui->menuAutoRead->setEnabled(false);
    w->textsInfo.clear();
    w->ui->widgetFull->clearData();
    w->ui->widgetOneLine->clearData();
}

void FileInfo::loadPiece(uintptr_t piece)
{
    if (pieceLoaded[piece])
        return;

    qint64 startpos = piece * PIECESIZE;
    mutexFile.lock();
    file.seek(startpos);
    if (piece < pieceLoadedSize - 1) {
        file.read(content + startpos, PIECESIZE);
    }
    else {
        file.read(content + startpos, file.size() - startpos);
    }
    mutexFile.unlock();

    pieceLoaded[piece] = true;
}

void FileInfo::detectCodec()
{
    file.seek(0);
    auto ba = file.read(3);
    QTextCodec *ncodec = QTextCodec::codecForUtfText(ba, nullptr);
    if (ncodec) {
        if (ncodec->name().contains("UTF-8")) {
            w->codecTriggered(w->ui->actionUTF_8);
            return;
        }
        else if (ncodec->name().contains("UTF-16")) {
            if (ncodec->name().contains('B'))
                w->codecTriggered(w->ui->actionUTF_16BE);
            else
                w->codecTriggered(w->ui->actionUTF_16);
            return;
        }
    }
    else {
        if (utf8Detect() == 1) {
            w->codecTriggered(w->ui->actionUTF_8);
            return;
        }
    }

    if (codec->name() == "UTF-8" || codec->name().startsWith("UTF-16"))
        w->codecTriggered(w->ui->actionGB18030);
}

void FileInfo::checkCurrentPiece()
{
    uintptr_t cp = currentPos / PIECESIZE;
    if (cp < pieceLoadedSize)
        loadPiece(cp);
    else
        return;

    cp++;
    if (cp < pieceLoadedSize)
        loadPiece(cp);
}

void FileInfo::saveReadPos()
{
    if (file.isOpen()) {
        w->settings->setValue(file.fileName() + "/?pos", QVariant::fromValue(currentPos));
    }
}

void FileInfo::startReading()
{
    w->ui->menuCodec->setEnabled(false);
    // w->ui->verticalScrollBar->setEnabled(false);
    nextSentencePos = content + currentPos;
    connect(&w->textToSpeech, &QTextToSpeech::stateChanged, this, &FileInfo::handleStateChanged);
    prepareSentence();
}

void FileInfo::stopReading()
{
    w->ui->menuCodec->setEnabled(true);
    // w->ui->verticalScrollBar->setEnabled(true);
    disconnect(&w->textToSpeech, 0, 0, 0);
    w->textToSpeech.stop();
}

void FileInfo::prepareSentence()
{
    QString s;
    for (;;) {
        if (nextSentencePos >= contentEnd || w->textsInfo.empty()) {
            QMessageBox::about(w, tr("提示"), tr("朗读完毕！"));
            w->ui->actionRead->setChecked(false);
            return;
        }
        auto it = w->textsInfo.begin();
        while (it != w->textsInfo.end()) {
            if (it->contentPos >= nextSentencePos) {
                break;
            }
            it++;
        }

        while (it != w->textsInfo.end()) {
            if (w->stopWord.contains(it->c)) {
                if (!s.isEmpty()) {
                    do {
                        it++;
                    } while (it != w->textsInfo.end() && w->stopWord.contains(it->c));

                    if (it != w->textsInfo.end())
                        nextSentencePos = it->contentPos;
                    else
                        nextSentencePos = WidgetOutput::getNextPos((--it)->contentPos);
                    w->textToSpeech.say(s);
                    return;
                }
            }
            else {
                s.append(it->c);
            }
            it++;
        }

        nextSentencePos = WidgetOutput::getNextPos((w->textsInfo.end() - 1)->contentPos);
        w->currentOutput->pageMoveDown();
        if (!s.isEmpty()) {
            w->textToSpeech.say(s);
            return;
        }
    }
}

void FileInfo::setTitle()
{
    QString title;
    auto it = mapIndex.upperBound(currentPos);
    if (it != mapIndex.begin()) {
        it--;
        title = it.value() + " - ";
    }

    w->setWindowTitle(title + file.fileName() + " - " + POPKC_TITLE);
}

void FileInfo::setCurrentPos(uintptr_t npos)
{
    currentPos = npos;
    if (!mapIndex.empty())
        setTitle();
}

void FileInfo::renewMapIndex()
{
    mapIndex.clear();
    if (w->db.isOpen()) {
        QSqlQuery gfid;
        gfid.prepare("SELECT ROWID FROM 'files' WHERE file = ?");
        gfid.bindValue(0, file.fileName());
        gfid.exec();
        int fileId;
        if (gfid.next()) {
            fileId = gfid.value(0).toInt();
        }
        else {
            QSqlQuery ifile;
            ifile.prepare("INSERT INTO 'files' (file) VALUES (?)");
            ifile.bindValue(0, w->fileInfo.file.fileName());
            fileId = ifile.lastInsertId().toInt();
        }
        QSqlQuery gi;
        gi.prepare("SELECT pos,string FROM 'index' WHERE file = ?");
        gi.bindValue(0, fileId);
        gi.exec();
        while (gi.next()) {
            mapIndex.insert(gi.value(0).toUInt(), gi.value(1).toString());
        }
    }
}

void FileInfo::iterateDirectory(bool isNext)
{
    auto fi = QFileInfo(file.fileName());
    auto d = fi.dir();
    auto fn = fi.fileName();
    QStringList sl;
    sl << "*.txt";
    auto el = d.entryList(sl, QDir::NoFilter, QDir::Name | QDir::IgnoreCase);
    QString prev;
    for (auto it = el.begin(); it != el.end(); it++) {
        if (*it == fn) {
            if (isNext) {
                it++;
                if (it != el.end()) {
                    loadFile(d.filePath(*it));
                }
            }
            else {
                if (!prev.isEmpty()) {
                    loadFile(d.filePath(prev));
                }
            }
            return;
        }
        prev = *it;
    }
}

char *FileInfo::findLastParaStart(char *cpos)
{
    assert(cpos > content);
    char *ocp = cpos;
    uintptr_t piece = (cpos - content) / PIECESIZE;
    if (piece >= 1)
        loadPiece(piece - 1);

    cpos--;
    intptr_t dis = cpos - content;
    bool nearStart;
    if (dis > 4096) {
        dis = 4096;
        nearStart = false;
    }
    else
        nearStart = true;

    if (codec->name().startsWith("UTF-16")) {
        QTextEncoder te(codec, QTextCodec::IgnoreHeader);
        QByteArray ba = te.fromUnicode(QString("\n"));
        QByteArray bar = te.fromUnicode(QString("\r"));
        for (;;) {
            char *p = myMemrchr(dis, cpos, ba[1]);
            if (!p) {
                if (nearStart)
                    return content;
                else
                    return nullptr;
            }

            if ((p - content) % 2 == 1) {
                if (*(p - 1) == ba[0]) {
                    if (cpos - p <= 2 || (cpos - p == 3 && !memcmp(bar.data(), p + 1, 2))) {
                        goto nextLoop;
                    }

                    return p + 1;
                }
                p--;
            }
        nextLoop:
            dis -= cpos - p;
            cpos = p;
        }
    }
    else {
        for (;;) {
            char *p = myMemrchr(dis, cpos, '\n');
            if (p) {
                if (cpos - p <= 1 || (cpos - p == 2 && p[1] == '\r')) {
                    dis -= cpos - p;
                    cpos = p;
                    continue;
                }
                assert(ocp - p <= 4096);
                return p + 1;
            }
            else if (nearStart)
                return content;
            else
                return nullptr;
        }
    }
}

int FileInfo::getCodecType()
{
    if (codec->name() == "UTF-8")
        return 0;
    else if (codec->name().startsWith("UTF-16"))
        return 1;
    else
        return 2;
}

void FileInfo::handleStateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Ready) {
        moveToNsp();
        prepareSentence();
    }
}

int FileInfo::utf8Detect()
{
    if (file.size() == 0)
        return 0;

    loadPiece(0);
    unsigned char *start = reinterpret_cast<unsigned char *>(content);
    unsigned char *end;
    if (file.size() < PIECESIZE)
        end = start + file.size();
    else
        end = start + PIECESIZE;

    bool f = false;
    do {
        if (*start >= 0x80) {
            f = true;
            int count;
            if (*start < 0xc0 || *start >= 0xfe)
                return 2;
            else if (*start < 0xe0)
                count = 1;
            else if (*start < 0xf0)
                count = 2;
            else if (*start < 0xf8)
                count = 3;
            else if (*start < 0xfc)
                count = 4;
            else
                count = 5;

            start++;
            while (start < end) {
                if (*start < 0x80 || *start >= 0xc0)
                    return 2;

                count--;
                if (count == 0)
                    break;
                start++;
            }
        }
        start++;
    } while (start < end);
    if (f)
        return 1;
    else
        return 0;
}

void FileInfo::moveToNsp()
{
    if (w->isOneLine) {
        currentPos = nextSentencePos - content;
        w->currentOutput->renewCache();
        w->currentOutput->update();
    }
    else {
        auto it = w->textsInfo.begin();
        int y = it->screenPos.y();
        int lc = 0;
        while (it != w->textsInfo.end()) {
            if (it->screenPos.y() != y) {
                lc++;
                y = it->screenPos.y();
            }

            if (it->contentPos >= nextSentencePos) {
                for (int i = 0; i < lc; i++) {
                    w->currentOutput->lineMoveDown();
                }
                break;
            }
            it++;
        }

        if (it == w->textsInfo.end()) {
            w->currentOutput->pageMoveDown();
        }
    }
    w->ui->verticalScrollBar->setValue(currentPos >> sliderShift);
}

//算法来自于glibc里的memrchr函数
char *myMemrchr(intptr_t len, char *memEnd, char c)
{
    uintptr_t l = sizeof(uintptr_t) - 1;
    uintptr_t lc, tl;
    while (len > 0 && (reinterpret_cast<uintptr_t>(memEnd) & l) != 0) {
        memEnd--;
        if (*memEnd == c)
            return memEnd;
        len--;
    }
    lc = c | (c << 8);
    lc |= lc << 16;
    if (sizeof(uintptr_t) == 4) {
        l = 0x7efefeff;
    }
    else {
        lc |= lc << 32;
        l = ((uintptr_t)0x7efefefe << 32) | 0xfefefeff;
    }

    uintptr_t *pl = reinterpret_cast<uintptr_t *>(memEnd);
    while (len >= (int)sizeof(uintptr_t)) {
        pl--;
        tl = *pl ^ lc;
        if ((((tl + l) ^ ~tl) & ~l) != 0) {
            if (sizeof(uintptr_t) == 8) {
                if (reinterpret_cast<char *>(pl)[7] == c)
                    return reinterpret_cast<char *>(pl) + 7;
                if (reinterpret_cast<char *>(pl)[6] == c)
                    return reinterpret_cast<char *>(pl) + 6;
                if (reinterpret_cast<char *>(pl)[5] == c)
                    return reinterpret_cast<char *>(pl) + 5;
                if (reinterpret_cast<char *>(pl)[4] == c)
                    return reinterpret_cast<char *>(pl) + 4;
            }
            if (reinterpret_cast<char *>(pl)[3] == c)
                return reinterpret_cast<char *>(pl) + 3;
            if (reinterpret_cast<char *>(pl)[2] == c)
                return reinterpret_cast<char *>(pl) + 2;
            if (reinterpret_cast<char *>(pl)[1] == c)
                return reinterpret_cast<char *>(pl) + 1;
            if (*reinterpret_cast<char *>(pl) == c)
                return reinterpret_cast<char *>(pl);
        }

        len -= sizeof(uintptr_t);
    }

    memEnd = reinterpret_cast<char *>(pl);
    while (len > 0) {
        memEnd--;
        if (*memEnd == c)
            return memEnd;
        len--;
    }
    return nullptr;
}
