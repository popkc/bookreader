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
#include "core/widgetoneline.h"
#include "core/widgetoutput.h"
#include "mainwindow.h"
#include <QApplication>

MainWindow *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("bookreader");
    a.setOrganizationName("popkc");
    QTranslator tl;
    tl.load("qt_zh_CN.qm");
    a.installTranslator(&tl);
    w = new MainWindow;
    w->init();
    if (w->isOneLine) {
        static_cast<WidgetOneLine *>(w->currentOutput)->adjustHeight();
        w->currentOutput->restoreState();
    }
    else
        w->showMaximized();
    w->init2();

    return a.exec();
}
