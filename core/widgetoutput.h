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
#pragma once

#include "pch.h"

class WidgetOutput : public QWidget
{
    Q_OBJECT

public:
    WidgetOutput(QWidget *parent);
    ~WidgetOutput();
    static char *getNextPos(char *cpos);
    virtual void changeCacheSize() = 0;

    virtual void lineMoveDown() = 0;
    virtual void lineMoveUp() = 0;
    virtual void pageMoveDown() = 0;
    virtual void pageMoveUp() = 0;
    virtual void randomMove(uintptr_t pos) = 0;
    virtual void renewCache() = 0;
    virtual void addOffset(int value) = 0;
    void saveState();
    void restoreState();
    void changeCurrentPos(uintptr_t pos);
    bool isEndChar(QChar &c);
    void init();
    void clearData();

    char *prevPage;
    char *prevLine;
    bool needRedraw;
    int offset;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    // void mouseDoubleClickEvent(QMouseEvent *event) override;

    // char *cpos;
    // int left,top,x,y;
    QPixmap cache;
    int cacheStartPos;
    QPainter painter;
};
