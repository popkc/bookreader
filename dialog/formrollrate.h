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
#ifndef FORMROLLRATE_H
#define FORMROLLRATE_H

#include <QWidget>

namespace Ui {
class FormRollRate;
}

class FormRollRate : public QWidget
{
    Q_OBJECT

public:
    explicit FormRollRate(QWidget *parent = 0);
    ~FormRollRate();
    void renewValue();

private slots:
    void on_horizontalSlider_valueChanged(int value);

private:
    Ui::FormRollRate *ui;
};

#endif // FORMROLLRATE_H
