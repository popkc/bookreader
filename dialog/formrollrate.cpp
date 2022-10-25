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
#include "formrollrate.h"
#include "ui_formrollrate.h"
#include "mainwindow.h"
#include "dialogconfig.h"

FormRollRate::FormRollRate(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormRollRate)
{
    ui->setupUi(this);
    ui->horizontalSlider->setMinimum(MIN_ROLLRATE);
    ui->horizontalSlider->setMaximum(MAX_ROLLRATE);
}

FormRollRate::~FormRollRate()
{
    delete ui;
}

void FormRollRate::renewValue()
{
    ui->horizontalSlider->setValue(w->rollRate);
}

void FormRollRate::on_horizontalSlider_valueChanged(int value)
{
	if(w)
		w->rollRate=value;
}
