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
#ifndef DIALOGSEARCH_H
#define DIALOGSEARCH_H

#include "pch.h"

namespace Ui {
class DialogSearch;
}

class DialogSearch : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSearch(QWidget *parent = 0);
    ~DialogSearch();
    void workFind();
    void workLoad();

    std::thread threadFind, threadLoad;
    std::atomic<bool> running;
    std::atomic<int> finishedCount;
    bool isFind;
    QByteArray baToFind;
signals:
    void found(char* pos);
    void process(char* pos);
protected:
    void closeEvent(QCloseEvent *);
private slots:
    void on_pushButtonFind_clicked();

    void onFound(char* pos);
    void onProcess(char* pos);
private:
    void discon();
    Ui::DialogSearch *ui;
    char *startPos;
};

#endif // DIALOGSEARCH_H
