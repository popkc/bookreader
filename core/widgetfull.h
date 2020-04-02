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
#pragma once

#include "core/widgetoutput.h"

class WidgetFull : public WidgetOutput
{
	Q_OBJECT

public:
	WidgetFull(QWidget *parent);
	~WidgetFull();
    virtual void changeCacheSize() override;

    virtual void lineMoveDown() override;
    virtual void lineMoveUp() override;
    virtual void pageMoveDown() override;
    virtual void pageMoveUp() override;
    virtual void randomMove(quint32 pos) override;
    virtual void renewCache() override;
    virtual void addOffset(int value) override;
    int lineCount;
	char *selectedStart, *selectedEnd;
protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

    void textCache(int start, int count, char* cpos);
    QList<char*> findLineStop(char* start, char* end);
    char* fillLine(char* end);
    int actualLines();
    char *getContentPosFromScreen(const QPoint &p);
	void drawSelected();
	void prepareCachePainter();

    QPoint pressPoint;
    bool dragging;
	QMenu *popupMenu;
private slots:
	void onActionCopy();
};
