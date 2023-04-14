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
#include "dialog/dialogindex.h"
#include "core/widgetoutput.h"
#include "dialogindexadvance.h"
#include "mainwindow.h"
#include "ui_dialogindex.h"

DialogIndex::DialogIndex(QWidget* parent)
    : QDialog(parent)
    , running(false)
    , ui(new Ui::DialogIndex)
{
    dialogIndexAdvance = nullptr;
    ui->setupUi(this);

    setupRegexps();

    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    // connect(this, &DialogIndex::indexFound, this, &DialogIndex::onIndexFound, Qt::QueuedConnection);
    // connect(this, &DialogIndex::process, this, &DialogIndex::onProcess, Qt::QueuedConnection);
}

DialogIndex::~DialogIndex()
{
    delete ui;
}

void DialogIndex::setupRegexps()
{
    QStringList sl = w->settings->value("?app/indexregexps", DEFAULT_REGEXPS).toStringList();
    regexps.resize(sl.size());
    for (int i = 0; i < sl.size(); i++) {
        regexps[i].setPattern(sl[i]);
    }
    regexps2 = regexps;
    maxWord = w->settings->value("?app/indexmaxword", DEFAULT_MAXWORD).toInt();
}

void DialogIndex::workIndex()
{
    int maxByte;
    if (w->fileInfo.codec->name() == "UTF-8")
        maxByte = maxWord * 3;
    else
        maxByte = maxWord * 2;

    char* p = w->fileInfo.content + startPos;
    char* np;
    lastPiece = (w->fileInfo.contentEnd - w->fileInfo.content + 1) / PIECESIZE;
    QTextDecoder td(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    if (w->fileInfo.codec->name().startsWith("UTF-16")) {
        utf16 = true;
        QTextEncoder te(w->fileInfo.codec, QTextCodec::IgnoreHeader);
        utf16n = te.fromUnicode("\n");
    }
    else
        utf16 = false;

    if (startPos != 0) {
        piece = startPos / PIECESIZE;
        p = findNextN(p);
        if (!p)
            goto end;
        p++;
    }
    else
        piece = 0;

    while ((np = findNextN(p))) {
        if (np != p && np - p <= maxByte) {
            QString s;
            if (utf16)
                s = td.toUnicode(p, np - p - 1);
            else
                s = td.toUnicode(p, np - p);

            s.remove('\r');
            for (const auto& r : regexps) {
                auto ma = r.match(s);
                if (ma.hasMatch()) {
                    emit indexFound(p, s, ma.capturedTexts());
                    break;
                }
            }
        }
        p = np + 1;
    }
end:;
    emit process(100);
    finishedCount--;
}

void DialogIndex::workLoad()
{
    int piece = startPos / PIECESIZE;
    int lastPiece = (w->fileInfo.contentEnd - w->fileInfo.content - 1) / PIECESIZE;
    do {
        w->fileInfo.loadPiece(piece);
        piece++;
    } while (piece <= lastPiece && running);
    this->finishedCount--;
}

void DialogIndex::init()
{
    /*ui->tableWidget->clearContents();
    auto sl = w->settings->value(w->fileInfo.file.fileName() + "/?indexpos").toStringList();
    auto sl2 = w->settings->value(w->fileInfo.file.fileName() + "/?indexstring").toStringList();
    int cou = sl.size();
    ui->tableWidget->setRowCount(cou);
    QTableWidgetItem* wi;
    for (int i = 0; i < cou; i++) {
        wi = new QTableWidgetItem(sl[i]);
        ui->tableWidget->setItem(i, 0, wi);
        if (i < sl2.size()) {
            wi = new QTableWidgetItem(sl2[i]);
            ui->tableWidget->setItem(i, 1, wi);
        }
    }
    changed = false;
    selectCurrentPos();*/

    if (w->db.isOpen()) {
        ui->tableWidget->clearContents();
        QSqlQuery gfid;
        gfid.prepare("SELECT ROWID FROM 'files' WHERE file = ?");
        gfid.bindValue(0, w->fileInfo.file.fileName());
        gfid.exec();
        if (!gfid.isActive())
            qDebug() << gfid.lastError();
        if (gfid.next()) {
            fileId = gfid.value(0).toInt();
        }
        else {
            QSqlQuery ifile;
            ifile.prepare("INSERT INTO 'files' (file) VALUES (?)");
            ifile.bindValue(0, w->fileInfo.file.fileName());
            if (!ifile.exec()) {
                qDebug() << ifile.lastError().text();
            }
            fileId = ifile.lastInsertId().toInt();
        }
        QSqlQuery gi;
        gi.prepare("SELECT pos,string FROM 'index' WHERE file = ? ORDER BY pos ASC");
        gi.bindValue(0, fileId);
        gi.exec();
        int tc = 0;
        while (gi.next()) {
            if (ui->tableWidget->rowCount() <= tc)
                ui->tableWidget->setRowCount(tc + 1);
            auto wi = new QTableWidgetItem(gi.value(0).toString());
            ui->tableWidget->setItem(tc, 0, wi);
            wi = new QTableWidgetItem(gi.value(1).toString());
            ui->tableWidget->setItem(tc, 1, wi);
            tc++;
        }
    }
    changed = false;
    selectCurrentPos();
}

void DialogIndex::save()
{
    if (w->db.isOpen() && changed) {
        /*int cou = ui->tableWidget->rowCount();
        QStringList sl1, sl2;
        for (int i = 0; i < cou; i++) {
            sl1 << ui->tableWidget->item(i, 0)->text();
            sl2 << ui->tableWidget->item(i, 1)->text();
        }
        w->settings->setValue(w->fileInfo.file.fileName() + "/?indexpos", sl1);
        w->settings->setValue(w->fileInfo.file.fileName() + "/?indexstring", sl2);*/

        w->db.transaction();
        QSqlQuery sqdl;
        sqdl.prepare("DELETE FROM 'index' WHERE file = ?");
        sqdl.bindValue(0, fileId);
        sqdl.exec();
        int c = ui->tableWidget->rowCount();
        QSqlQuery sqi;
        sqi.prepare("INSERT INTO 'index' (file,pos,string) VALUES (?,?,?)");
        sqi.bindValue(0, fileId);
        for (int i = 0; i < c; i++) {
            sqi.bindValue(1, ui->tableWidget->item(i, 0)->text());
            sqi.bindValue(2, ui->tableWidget->item(i, 1)->text());
            sqi.exec();
        }
        sqdl.finish();
        sqi.finish();
        w->db.commit();
    }
}

void DialogIndex::on_pushButtonAdvance_clicked()
{
    if (!dialogIndexAdvance) {
        dialogIndexAdvance = new DialogIndexAdvance(this);
    }
    dialogIndexAdvance->init();
    dialogIndexAdvance->exec();
}

void DialogIndex::on_pushButtonCreateIndex_clicked()
{
    if (!running) {
        if (regexps.isEmpty()) {
            QMessageBox::warning(this, tr("错误"), tr("没有有效的正则表达式。"));
            return;
        }

        isContinue = 0;

        if (ui->tableWidget->rowCount() > 1) {
            int r = QMessageBox::question(this, tr("提示"),
                tr("目前已经有索引存在，请问是否要清空？\n"
                   "选Yes，则清空现有索引，再重新建立索引。\n"
                   "选No，则保留现有索引，并在现有索引的最后处继续建立索引。\n"
                   "选Cancel，则不进行任何操作。"),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (r == QMessageBox::Yes) {
                startPos = 0;
                ui->tableWidget->clearContents();
                ui->tableWidget->setRowCount(0);
            }
            else if (r == QMessageBox::No) {
                startPos = ui->tableWidget->item(ui->tableWidget->rowCount() - 1, 0)->text().toUInt();
                isContinue = ui->tableWidget->rowCount();
            }
            else
                return;
        }
        else
            startPos = 0;

        mapMlist.clear();
        mlistlist.clear();
        changed = true;

        ui->pushButtonClear->setEnabled(false);
        ui->pushButtonDel->setEnabled(false);
        ui->pushButtonAdvance->setEnabled(false);
        ui->pushButtonCreateIndex->setText(tr("停止"));
        running = true;
        finishedCount = 2;
        connect(this, &DialogIndex::indexFound, this, &DialogIndex::onIndexFound, Qt::QueuedConnection);
        connect(this, &DialogIndex::process, this, &DialogIndex::onProcess, Qt::QueuedConnection);
        std::thread(&DialogIndex::workIndex, this).detach();
        std::thread(&DialogIndex::workLoad, this).detach();

        if (isContinue) {
            for (int i = 0; i < ui->tableWidget->rowCount(); i++) {
                auto s = ui->tableWidget->item(i, 1)->text();
                for (auto& r : regexps2) {
                    auto ma = r.match(s);
                    if (ma.hasMatch()) {
                        testMlist(ma.capturedTexts());
                        break;
                    }
                }
            }
        }
    }
    else {
        discon();
        ui->progressBarIndex->setValue(0);

        while (finishedCount > 0) {
            QThread::usleep(10);
        }
    }
}

void DialogIndex::testMlist(const QStringList& mlist)
{
    bool found = false;
    for (auto& p : mapMlist) {
        if (p.first.size() == mlist.size()) {
            int count = 0;
            for (int i = 0; i < mlist.size(); i++) {
                if (p.first[i] == mlist[i])
                    count++;
            }

            if (count >= 2) {
                found = true;
                p.second++;
                break;
            }
        }
    }

    if (!found) {
        mapMlist[mlist] = 1;
    }
    mlistlist.push_back(mlist);
}

void DialogIndex::onIndexFound(char* pos, const QString& s, const QStringList& mlist)
{
    if (!running)
        return;

    QString n = QString::number(pos - w->fileInfo.content);
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);
    QTableWidgetItem* twi = new QTableWidgetItem(n);
    ui->tableWidget->setItem(row, 0, twi);
    twi = new QTableWidgetItem(s);
    ui->tableWidget->setItem(row, 1, twi);

    testMlist(mlist);
}

void DialogIndex::onProcess(int value)
{
    if (!running)
        return;

    if (value < 100)
        ui->progressBarIndex->setValue(value);
    else {
        discon();
        ui->progressBarIndex->setValue(100);

        std::pair<const QStringList, int>* pp = nullptr;
        for (auto& p : mapMlist) {
            if (!pp)
                pp = &p;
            else {
                if (pp->second < p.second)
                    pp = &p;
            }
        }

        if (pp && w->settings->value("?app/quza", DEFAULT_QUZA).toBool()) {
            const auto& ml = pp->first;
            for (int i = 0; i < mlistlist.size();) {
                if (mlistlist[i].size() == ml.size()) {
                    int c = 0;
                    for (int j = 0; j < ml.size(); j++) {
                        if (mlistlist[i][j] == ml[j])
                            c++;
                    }

                    if (c >= 2) {
                        i++;
                        continue;
                    }
                }
                mlistlist.removeAt(i);
                ui->tableWidget->removeRow(i);
            }
        }

        QMessageBox::about(this, tr("提示"), tr("索引建立完毕！"));
        ui->progressBarIndex->setValue(0);
        while (finishedCount > 0) {
            QThread::usleep(10);
        }
    }
}

char* DialogIndex::findNextN(char* pos)
{
    if (pos >= w->fileInfo.contentEnd)
        return nullptr;

    for (;;) {
        while (!w->fileInfo.pieceLoaded[piece]) {
            QThread::usleep(10);
        }

        if (piece < lastPiece) {
            quint32 mp = (piece + 1) * PIECESIZE;
            if (utf16) {
                int dis = pos - w->fileInfo.content;
                while ((pos = (char*)memchr(pos, utf16n[1], mp - dis))) {
                    if ((dis & 1) == 1) {
                        if (*(pos - 1) == utf16n[0])
                            return pos;
                    }
                    pos++;
                    dis = pos - w->fileInfo.content;
                }
            }
            else {
                pos = (char*)memchr(pos, '\n', mp - (pos - w->fileInfo.content));
                if (pos)
                    return pos;
            }

            if (!running)
                return nullptr;

            pos = w->fileInfo.content + mp;
            piece++;
            emit process((pos - w->fileInfo.content - startPos) * 100 / (w->fileInfo.file.size() - startPos));
        }
        else {
            if (utf16) {
                while ((pos = (char*)memchr(pos, utf16n[1], w->fileInfo.contentEnd - pos))) {
                    if (((pos - w->fileInfo.content) & 1) == 1) {
                        if (*(pos - 1) == utf16n[0])
                            return pos;
                    }
                    pos++;
                }
            }
            else {
                return (char*)memchr(pos, '\n', w->fileInfo.contentEnd - pos);
            }
            return nullptr;
        }
    }
}

void DialogIndex::discon()
{
    running = false;
    disconnect(this, &DialogIndex::indexFound, 0, 0);
    disconnect(this, &DialogIndex::process, 0, 0);
    ui->pushButtonClear->setEnabled(true);
    ui->pushButtonDel->setEnabled(true);
    ui->pushButtonAdvance->setEnabled(true);
    ui->pushButtonCreateIndex->setText(tr("建立索引"));
}

void DialogIndex::selectCurrentPos()
{
    int c = ui->tableWidget->rowCount();
    if (c == 0)
        return;
    int i;
    for (i = 0; i < c; i++) {
        if (w->fileInfo.currentPos < ui->tableWidget->item(i, 0)->text().toUInt()) {
            if (i > 0)
                i--;
            break;
        }
    }
    if (i == c)
        i--;
    ui->tableWidget->selectRow(0);
    ui->tableWidget->selectRow(i);
    ui->tableWidget->setFocus();
}

void DialogIndex::on_pushButtonClear_clicked()
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    changed = true;
}

void DialogIndex::on_pushButtonDel_clicked()
{
    auto ilist = ui->tableWidget->selectedItems();
    if (ilist.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请选择要删除的条目。"));
        return;
    }
    changed = true;
    std::set<int> si;
    for (auto wi : ilist) {
        si.insert(wi->row());
    }
    auto it = si.end();
    if (it != si.begin()) {
        do {
            it--;
            ui->tableWidget->removeRow(*it);
        } while (it != si.begin());
    }
}

void DialogIndex::on_tableWidget_cellDoubleClicked(int row, int)
{
    quint32 cp = ui->tableWidget->item(row, 0)->text().toUInt();
    if (cp >= w->fileInfo.file.size()) {
        QMessageBox::warning(this, tr("错误"), tr("索引位置超过了文件大小，请重新建立索引！"));
        return;
    }
    w->currentOutput->changeCurrentPos(cp);
}

void DialogIndex::on_pushButtonCurrentPos_clicked()
{
    if (ui->tableWidget->rowCount() == 0) {
        QMessageBox::warning(this, tr("错误"), tr("没有有效的索引。"));
        return;
    }

    selectCurrentPos();
}
