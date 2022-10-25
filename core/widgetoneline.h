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
struct PaintInfo;

class WidgetOneLine : public WidgetOutput
{
	Q_OBJECT

public:
	WidgetOneLine(QWidget *parent);
	~WidgetOneLine();
    virtual void changeCacheSize() override;

    virtual void lineMoveDown() override;
    virtual void lineMoveUp() override;
    virtual void pageMoveDown() override;
    virtual void pageMoveUp() override;
    virtual void randomMove(quint32 pos) override;
    virtual void renewCache() override;
    virtual void addOffset(int value) override;
	void adjustHeight();
	void init();
	void endHide();
	void adjustPos();

	PaintInfo *pi;
	bool hidding;
protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void leaveEvent(QEvent *event);
	void prepareCachePainter();
	void textCache(int start, int length, char *cpos);
	QString getLastWord(int codecType, char *&p);
	int cacheAbsDistance(int start, int end);
	void textRemain(int start, int end, int cacheStart);

	enum {
		PressMiddle,
		PressRight
	} pressState;
	QPoint pressMousePos;
	QRect pressRect;
	int spaceRemain;
	int currentSpace;
	int cacheRealWidth;
	QMenu popupMenu;
	QAction *actionCopy;
	enum {
		HIDEPOS_NONE,
		HIDEPOS_LEFT,
		HIDEPOS_RIGHT,
		HIDEPOS_TOP,
		HIDEPOS_BOTTOM
	} hidePos;
private slots:
	void onActionCopy();
};
