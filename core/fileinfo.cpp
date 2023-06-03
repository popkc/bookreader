/*
Copyright (C) 2020 popkc(popkcer at gmail dot com)
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

FileInfo::FileInfo(QObject* parent)
    : QObject(parent)
{
    content = nullptr;
    pieceLoaded = nullptr;
}

FileInfo::~FileInfo()
{
    if (content)
        delete[] content;

    if (pieceLoaded)
        delete[] pieceLoaded;
}

void FileInfo::loadFile(const QString& fn)
{
    file.setFileName(fn);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(w, tr("错误"), tr("打开文件失败，文件名：") + fn);
        return;
    }

    if (file.size() > 2147483647) {
        QMessageBox::warning(w, tr("错误"), tr("文件大小不能超过2GB。"));
        return;
    }

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
    memset(pieceLoaded, 0, sizeof(std::atomic<bool>) * pieceLoadedSize);

    if (file.size() != 0) {
        content = new char[file.size()];
        contentEnd = content + file.size();
        w->ui->verticalScrollBar->setMaximum(file.size() - 1);
    }
    else
        w->ui->verticalScrollBar->setMaximum(0);

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
    if (file.size() <= currentPos)
        currentPos = 0;
    w->ui->verticalScrollBar->setValue(currentPos);

    w->currentOutput->needRedraw = true;
    w->currentOutput->update();
    w->setWindowTitle(POPKC_TITLE + " - " + fn);
    w->ui->menuJump->setEnabled(true);
    w->ui->menuAutoRead->setEnabled(true);
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
    w->currentOutput->textsInfo.clear();
}

void FileInfo::loadPiece(quint32 piece)
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
    QTextCodec* ncodec = QTextCodec::codecForUtfText(ba, nullptr);
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
    quint32 cp = currentPos / PIECESIZE;
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
        w->settings->setValue(file.fileName() + "/?pos", currentPos);
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
        if (nextSentencePos >= contentEnd || w->currentOutput->textsInfo.empty()) {
            QMessageBox::about(w, tr("提示"), tr("朗读完毕！"));
            w->ui->actionRead->setChecked(false);
            return;
        }
        auto it = w->currentOutput->textsInfo.begin();
        while (it != w->currentOutput->textsInfo.end()) {
            if (it->contentPos >= nextSentencePos) {
                break;
            }
            it++;
        }

        while (it != w->currentOutput->textsInfo.end()) {
            if (w->stopWord.contains(it->c)) {
                if (!s.isEmpty()) {
                    do {
                        it++;
                    } while (it != w->currentOutput->textsInfo.end() && w->stopWord.contains(it->c));

                    if (it != w->currentOutput->textsInfo.end())
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

        nextSentencePos = WidgetOutput::getNextPos((w->currentOutput->textsInfo.end() - 1)->contentPos);
        w->currentOutput->pageMoveDown();
        if (!s.isEmpty()) {
            w->textToSpeech.say(s);
            return;
        }
    }
}

char* FileInfo::findLastParaStart(char* cpos)
{
    quint32 piece = (cpos - content) / PIECESIZE;
    if (piece >= 1)
        loadPiece(piece - 1);

    cpos--;
    int dis = cpos - content;
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
            char* p = myMemrchr(dis, cpos, ba[1]);
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
            char* p = myMemrchr(dis, cpos, '\n');
            if (p) {
                if (cpos - p <= 1 || (cpos - p == 2 && p[1] == '\r')) {
                    dis -= cpos - p;
                    cpos = p;
                    continue;
                }
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
    unsigned char* start = reinterpret_cast<unsigned char*>(content);
    unsigned char* end;
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
        auto it = w->currentOutput->textsInfo.begin();
        int y = it->screenPos.y();
        int lc = 0;
        while (it != w->currentOutput->textsInfo.end()) {
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

        if (it == w->currentOutput->textsInfo.end()) {
            w->currentOutput->pageMoveDown();
        }
    }
    w->ui->verticalScrollBar->setValue(currentPos);
}

//算法来自于glibc里的memrchr函数
char* myMemrchr(int len, char* memEnd, char c)
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

    uintptr_t* pl = reinterpret_cast<uintptr_t*>(memEnd);
    while (len >= (int)sizeof(uintptr_t)) {
        pl--;
        tl = *pl ^ lc;
        if ((((tl + l) ^ ~tl) & ~l) != 0) {
            if (sizeof(uintptr_t) == 8) {
                if (reinterpret_cast<char*>(pl)[7] == c)
                    return reinterpret_cast<char*>(pl) + 7;
                if (reinterpret_cast<char*>(pl)[6] == c)
                    return reinterpret_cast<char*>(pl) + 6;
                if (reinterpret_cast<char*>(pl)[5] == c)
                    return reinterpret_cast<char*>(pl) + 5;
                if (reinterpret_cast<char*>(pl)[4] == c)
                    return reinterpret_cast<char*>(pl) + 4;
            }
            if (reinterpret_cast<char*>(pl)[3] == c)
                return reinterpret_cast<char*>(pl) + 3;
            if (reinterpret_cast<char*>(pl)[2] == c)
                return reinterpret_cast<char*>(pl) + 2;
            if (reinterpret_cast<char*>(pl)[1] == c)
                return reinterpret_cast<char*>(pl) + 1;
            if (*reinterpret_cast<char*>(pl) == c)
                return reinterpret_cast<char*>(pl);
        }

        len -= sizeof(uintptr_t);
    }

    memEnd = reinterpret_cast<char*>(pl);
    while (len > 0) {
        memEnd--;
        if (*memEnd == c)
            return memEnd;
        len--;
    }
    return nullptr;
}
