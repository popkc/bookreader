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
#include "dialog/dialogsearch.h"
#include "core/widgetfull.h"
#include "mainwindow.h"
#include "ui_dialogsearch.h"
#include "ui_mainwindow.h"

DialogSearch::DialogSearch(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSearch)
{
    ui->setupUi(this);
    isFind = true;
}

DialogSearch::~DialogSearch()
{
    delete ui;
}

void DialogSearch::workFind()
{
    QByteArrayMatcher bam(baToFind);
    char *pos = startPos;
    char *endPos;
    int lastPiece = (w->fileInfo.contentEnd - w->fileInfo.content - 1) / PIECESIZE;
    int piece = (pos - w->fileInfo.content) / PIECESIZE;
    do {
        while (!w->fileInfo.pieceLoaded[piece]) {
            // qDebug("wait for piece");
            QThread::usleep(10);
        }

        if (piece == lastPiece)
            endPos = w->fileInfo.contentEnd;
        else
            endPos = w->fileInfo.content + PIECESIZE * (piece + 1);

        int r = bam.indexIn(pos, endPos - pos);
        if (r != -1) {
            this->finishedCount--;
            emit this->found(pos + r);
            return;
        }
        emit this->process(endPos);
        pos = endPos - baToFind.size() + 1;
        piece++;
    } while (piece <= lastPiece && running);
    this->finishedCount--;
    emit this->found(NULL);
}

void DialogSearch::workLoad()
{
    int piece = (startPos - w->fileInfo.content) / PIECESIZE;
    int lastPiece = (w->fileInfo.contentEnd - w->fileInfo.content - 1) / PIECESIZE;
    do {
        w->fileInfo.loadPiece(piece);
        piece++;
    } while (piece <= lastPiece && running);
    this->finishedCount--;
}

void DialogSearch::closeEvent(QCloseEvent *)
{
    if (!isFind) {
        on_pushButtonFind_clicked();
    }
}

void DialogSearch::on_pushButtonFind_clicked()
{
    if (isFind) {
        QString s = ui->lineEdit->text();
        if (s.isEmpty()) {
            QMessageBox::warning(this, tr("错误"), tr("请输入要查找的字符串！"));
            return;
        }
        if (w->fileInfo.file.size() < 1) {
            QMessageBox::warning(this, tr("错误"), tr("文件为空！"));
            return;
        }

        if (ui->radioButtonNext->isChecked()) {
            if (w->isOneLine || static_cast<WidgetFull *>(w->currentOutput)->lineCount <= 3) {
                if (!w->textsInfo.isEmpty()) {
                    startPos = w->textsInfo.back().contentPos;
                    goto spfound;
                }
            }
            else {
                auto it = w->textsInfo.begin();
                if (it != w->textsInfo.end()) {
                    int y = it->screenPos.y();
                    it++;
                    while (it != w->textsInfo.end()) {
                        if (it->screenPos.y() != y) {
                            it++;
                            if (it != w->textsInfo.end()) {
                                startPos = it->contentPos;
                                goto spfound;
                            }
                            break;
                        }
                        it++;
                    }
                }
            }
            QMessageBox::warning(this, tr("提示"), tr("已经到达文件尾部。"));
            return;
        spfound:;
        }
        else
            startPos = w->fileInfo.content;

        QTextEncoder te(w->fileInfo.codec, QTextCodec::IgnoreHeader);
        baToFind = te.fromUnicode(s);
        running = true;
        finishedCount = 2;
        isFind = false;
        ui->pushButtonFind->setText(tr("停止"));
        connect(this, &DialogSearch::found, this, &DialogSearch::onFound, Qt::QueuedConnection);
        connect(this, &DialogSearch::process, this, &DialogSearch::onProcess, Qt::QueuedConnection);
        threadFind = std::thread(&DialogSearch::workFind, this);
        threadLoad = std::thread(&DialogSearch::workLoad, this);
        threadLoad.detach();
        threadFind.detach();
    }
    else {
        discon();
        while (finishedCount > 0) {
            QThread::usleep(10);
        }
    }
}

void DialogSearch::onFound(char *pos)
{
    if (!running)
        return;

    discon();
    if (!pos) {
        QMessageBox::about(this, tr("查找完毕"), tr("没有找到！"));
    }
    else {
        if (w->ui->actionRead->isChecked())
            w->ui->actionRead->setChecked(false);
        w->currentOutput->randomMove(pos - w->fileInfo.content);
        w->ui->verticalScrollBar->setValue(w->fileInfo.currentPos);
        if (!w->isOneLine) {
            WidgetFull *wf = reinterpret_cast<WidgetFull *>(w->currentOutput);
            wf->selectedStart = pos;
            wf->selectedEnd = pos + baToFind.size() - 1;
        }
    }

    while (finishedCount > 0) {
        // qDebug("thread waiting");
        QThread::usleep(10);
    }
}

void DialogSearch::onProcess(char *pos)
{
    if (!running)
        return;

    int v = (pos - w->fileInfo.content) * 100 / (w->fileInfo.contentEnd - w->fileInfo.content);
    ui->progressBar->setValue(v);
}

void DialogSearch::discon()
{
    running = false;
    disconnect(this, &DialogSearch::found, 0, 0);
    disconnect(this, &DialogSearch::process, 0, 0);
    isFind = true;
    ui->pushButtonFind->setText(tr("查找"));
    ui->progressBar->setValue(0);
}
