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
#include "dialog/dialogindexadvance.h"
#include "ui_dialogindexadvance.h"
#include "mainwindow.h"
#include "dialogindex.h"

DialogIndexAdvance::DialogIndexAdvance(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogIndexAdvance)
{
    ui->setupUi(this);
}

DialogIndexAdvance::~DialogIndexAdvance()
{
    delete ui;
}

void DialogIndexAdvance::init()
{
    ui->listWidgetIndex->clear();
    QStringList sl=w->settings->value("?app/indexregexps", DEFAULT_REGEXPS).toStringList();
    ui->listWidgetIndex->addItems(sl);
    ui->spinBoxMaxWord->setValue(w->settings->value("?app/indexmaxword", DEFAULT_MAXWORD).toInt());
}

void DialogIndexAdvance::on_pushButtonAdd_clicked()
{
    bool ok;
    QString s=QInputDialog::getText(this, tr("添加"), tr("请输入要添加的正则表达式"), QLineEdit::Normal, QString(), &ok);
    if(ok) {
        ui->listWidgetIndex->addItem(s);
    }
}

void DialogIndexAdvance::on_pushButtonModify_clicked()
{
    auto item=ui->listWidgetIndex->currentItem();
    if(item) {
        bool ok;
        QString s=QInputDialog::getText(this, tr("修改"), QString(), QLineEdit::Normal, item->text(), &ok);
        if(ok) {
            item->setText(s);
        }
    }
    else {
        QMessageBox::warning(this, tr("错误"), tr("请选择要修改的条目。"));
    }
}

void DialogIndexAdvance::on_pushButtonDel_clicked()
{
    auto item=ui->listWidgetIndex->currentItem();
    if(item)
        delete item;
    else
        QMessageBox::warning(this, tr("错误"), tr("请选择要删除的条目。"));
}

void DialogIndexAdvance::on_pushButtonDefault_clicked()
{
    ui->listWidgetIndex->clear();
    ui->listWidgetIndex->addItems(DEFAULT_REGEXPS);
    ui->spinBoxMaxWord->setValue(DEFAULT_MAXWORD);
}

void DialogIndexAdvance::on_pushButtonCancel_clicked()
{
    this->reject();
}

void DialogIndexAdvance::on_pushButtonOk_clicked()
{
    int c=ui->listWidgetIndex->count();
    QStringList sl;
    for(int i=0; i<c; i++) {
        sl << ui->listWidgetIndex->item(i)->text();
    }
    w->settings->setValue("?app/indexregexps", sl);
    w->settings->setValue("?app/indexmaxword", ui->spinBoxMaxWord->value());
    static_cast<DialogIndex*>(parent())->setupRegexps();
    this->accept();
}

void DialogIndexAdvance::on_pushButtonTest_clicked()
{
    int c=ui->listWidgetIndex->count();
    QVector<QRegExp> re(c);
    for(int i=0; i<c; i++) {
        re[i].setPattern(ui->listWidgetIndex->item(i)->text());
    }

    QString s=ui->lineEditTest->text();
    for(const QRegExp & r : re) {
        if(r.indexIn(s)!=-1) {
            QMessageBox::about(this, tr("提示"), tr("匹配成功！"));
            return;
        }
    }
    QMessageBox::about(this, tr("提示"), tr("匹配失败。"));
}
