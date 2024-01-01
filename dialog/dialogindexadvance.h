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
#ifndef DIALOGINDEXADVANCE_H
#define DIALOGINDEXADVANCE_H

#include "pch.h"

namespace Ui {
class DialogIndexAdvance;
}

class DialogIndexAdvance : public QDialog
{
    Q_OBJECT

public:
    explicit DialogIndexAdvance(QWidget *parent = 0);
    ~DialogIndexAdvance();
    void init();

private slots:
    void on_pushButtonAdd_clicked();

    void on_pushButtonModify_clicked();

    void on_pushButtonDel_clicked();

    void on_pushButtonDefault_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonOk_clicked();

    void on_pushButtonTest_clicked();

private:
    Ui::DialogIndexAdvance *ui;
};

#endif // DIALOGINDEXADVANCE_H
