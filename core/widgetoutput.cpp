﻿/*
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
#include "core/widgetoutput.h"
#include "core/widgetfull.h"
#include "mainwindow.h"
#include "pch.h"
#include "ui_mainwindow.h"

WidgetOutput::WidgetOutput(QWidget* parent)
    : QWidget(parent)
{
    needRedraw = false;
    prevLine = nullptr;
    prevPage = nullptr;
}

WidgetOutput::~WidgetOutput()
{
}

void WidgetOutput::resizeEvent(QResizeEvent*)
{
    changeCacheSize();
}

void WidgetOutput::wheelEvent(QWheelEvent* event)
{
    if (w->ui->actionRead->isChecked() || !w->fileInfo.file.isOpen())
        return;

    QPoint pt = event->angleDelta();
    if (pt.y() > 0) {
        if (w->isOneLine)
            pageMoveUp();
        else
            lineMoveUp();
    }
    else if (pt.y() < 0) {
        if (w->isOneLine)
            pageMoveDown();
        else
            lineMoveDown();
    }
    else {
        event->ignore();
        return;
    }
    event->accept();
    w->ui->verticalScrollBar->setValue(w->fileInfo.currentPos);
}

char* WidgetOutput::getNextPos(char* cpos)
{
    QTextDecoder decoder(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    for (;;) {
        QString s = decoder.toUnicode(cpos, 1);
        cpos++;
        if (!s.isEmpty())
            return cpos;
    }
}

void WidgetOutput::saveState()
{
    if (w->isOneLine)
        w->settings->setValue("?oneline/geometry", w->saveGeometry());
    else
        w->settings->setValue("?fullmode/geometry", w->saveGeometry());
}

void WidgetOutput::restoreState()
{
    if (w->isOneLine) {
        w->showNormal();
        w->restoreGeometry(w->settings->value("?oneline/geometry").toByteArray());
        if (w->width() < 10)
            w->resize(10, w->height());
    }
    else {
        w->showMaximized();
        w->restoreGeometry(w->settings->value("?fullmode/geometry").toByteArray());
    }
}

void WidgetOutput::changeCurrentPos(quint32 pos)
{
    w->fileInfo.currentPos = pos;
    needRedraw = true;
    prevLine = nullptr;
    prevPage = nullptr;
    update();
}

bool WidgetOutput::isEndChar(QChar& c)
{
    switch (c.unicode()) {
    case L'.':
    case L':':
    case L'"':
    case L';':
    case L'?':
    case L'!':
    case L'\'':
    case L')':
    case L'。':
    case L'？':
    case L'！':
    case L'”':
    case L'；':
    case L'’':
    case L'）':
    case L'…':
    case L'—':
    case L'」':
        return true;
    default:
        return false;
    }
}
