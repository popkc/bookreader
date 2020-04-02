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
#include "mainwindow.h"
#include <QApplication>
#include "core/widgetoutput.h"
#include "core/widgetoneline.h"

MainWindow *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator tl;
    tl.load("qt_zh_CN.qm");
    a.installTranslator(&tl);
    w=new MainWindow;
    w->init();
	w->currentOutput->restoreState();
	if(w->isOneLine)
		static_cast<WidgetOneLine*>(w->currentOutput)->adjustHeight();
    w->currentOutput->changeCacheSize();
	w->init2();

    return a.exec();;
}
