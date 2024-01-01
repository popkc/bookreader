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
#include "dialog/dialogconfig.h"
#include "core/widgetoutput.h"
#include "mainwindow.h"
#include "ui_dialogconfig.h"

DialogConfig::DialogConfig(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DialogConfig)
{
    ui->setupUi(this);
    ui->horizontalSliderRollRate->setMinimum(MIN_ROLLRATE);
    ui->horizontalSliderRollRate->setMaximum(MAX_ROLLRATE);

    auto vlist = w->textToSpeech.availableVoices();
    for (const QVoice& v : vlist) {
        ui->comboBoxVoice->addItem(v.name());
    }
}

DialogConfig::~DialogConfig()
{
    delete ui;
}

void DialogConfig::init()
{
    frontColor = w->paintInfo.fontcolor.color();
    backColor = w->paintInfo.background.color();
    font = w->paintInfo.font;

    int fontr, fontg, fontb, bgr, bgg, bgb;
    fontr = w->settings->value("?oneline/fontr", ONELINE_FONTR).toInt();
    fontg = w->settings->value("?oneline/fontg", ONELINE_FONTG).toInt();
    fontb = w->settings->value("?oneline/fontb", ONELINE_FONTB).toInt();
    bgr = w->settings->value("?oneline/bgr", ONELINE_BGR).toInt();
    bgg = w->settings->value("?oneline/bgg", ONELINE_BGG).toInt();
    bgb = w->settings->value("?oneline/bgb", ONELINE_BGB).toInt();
    olFrontColor.setRgb(fontr, fontg, fontb);
    olBackColor.setRgb(bgr, bgg, bgb);
    olFont.setFamily(w->settings->value("?oneline/fontfamily", ONELINE_FONTFAMALY).toString());
    olFont.setPointSize(w->settings->value("?oneline/fontsize", ONELINE_FONTPOINTSIZE).toInt());
    olFont.setWeight(w->settings->value("?oneline/fontweight", ONELINE_FONTPOINTWEIGHT).toInt());
    olFont.setItalic(w->settings->value("?oneline/fontitalic", ONELINE_FONTITALIC).toBool());
    olFont.setStyleName(w->settings->value("?oneline/fontstyle", ONELINE_FONTSTYLE).toString());

    ui->spinBoxLineSpace->setValue(w->paintInfo.linespace);
    ui->spinBoxPadding->setValue(w->paintInfo.padding);
    ui->checkBoxSmartReturn->setChecked(w->settings->value("?app/smartreturn", DEFAULT_SMARTRETURN).toBool());

    ui->spinBoxOLLineSpace->setValue(w->settings->value("?oneline/linespace", ONELINE_LINESPACE).toUInt());
    ui->spinBoxOLPadding->setValue(w->settings->value("?oneline/padding", ONELINE_PADDING).toUInt());
    ui->checkBoxSameAsFull->setChecked(w->settings->value("?oneline/sameasfull", ONELINE_SAMEASFULL).toBool());

    auto vlist = w->textToSpeech.availableVoices();
    QString s = w->settings->value("?voice/voice").toString();
    if (!s.isEmpty()) {
        for (int i = 0; i < vlist.size(); i++) {
            if (vlist[i].name() == s) {
                ui->comboBoxVoice->setCurrentIndex(i);
                goto voiceFound;
            }
        }
    }

    ui->comboBoxVoice->setCurrentIndex(0);
voiceFound:;
    ui->horizontalSliderVolume->setValue(w->settings->value("?voice/volume", DEFAULTVOLUME).toDouble() * 100);
    ui->horizontalSliderRate->setValue(w->settings->value("?voice/rate", DEFAULTRATE).toDouble() * 100);
    ui->horizontalSliderPitch->setValue(w->settings->value("?voice/pitch", DEFAULTPITCH).toDouble() * 100);
    on_horizontalSliderVolume_valueChanged(ui->horizontalSliderVolume->value());
    on_horizontalSliderRate_valueChanged(ui->horizontalSliderRate->value());
    on_horizontalSliderPitch_valueChanged(ui->horizontalSliderPitch->value());

    ui->horizontalSliderRollRate->setValue(w->rollRate);
}

void DialogConfig::save()
{
    w->settings->setValue("?app/fontr", frontColor.red());
    w->settings->setValue("?app/fontg", frontColor.green());
    w->settings->setValue("?app/fontb", frontColor.blue());
    w->settings->setValue("?app/bgr", backColor.red());
    w->settings->setValue("?app/bgg", backColor.green());
    w->settings->setValue("?app/bgb", backColor.blue());
    w->settings->setValue("?app/fontfamily", font.family());
    w->settings->setValue("?app/fontsize", font.pointSize());
    w->settings->setValue("?app/fontstyle", font.styleName());
    w->settings->setValue("?app/fontweight", font.weight());
    w->settings->setValue("?app/fontitalic", font.italic());
    w->settings->setValue("?app/linespace", ui->spinBoxLineSpace->value());
    w->settings->setValue("?app/padding", ui->spinBoxPadding->value());
    w->settings->setValue("?app/rollrate", ui->horizontalSliderRollRate->value());
    w->settings->setValue("?app/smartreturn", ui->checkBoxSmartReturn->isChecked());

    w->settings->setValue("?oneline/fontr", olFrontColor.red());
    w->settings->setValue("?oneline/fontg", olFrontColor.green());
    w->settings->setValue("?oneline/fontb", olFrontColor.blue());
    w->settings->setValue("?oneline/bgr", olBackColor.red());
    w->settings->setValue("?oneline/bgg", olBackColor.green());
    w->settings->setValue("?oneline/bgb", olBackColor.blue());
    w->settings->setValue("?oneline/fontfamily", olFont.family());
    w->settings->setValue("?oneline/fontsize", olFont.pointSize());
    w->settings->setValue("?oneline/fontstyle", olFont.styleName());
    w->settings->setValue("?oneline/fontweight", olFont.weight());
    w->settings->setValue("?oneline/fontitalic", olFont.italic());
    w->settings->setValue("?oneline/linespace", ui->spinBoxOLLineSpace->value());
    w->settings->setValue("?oneline/padding", ui->spinBoxOLPadding->value());
    w->settings->setValue("?oneline/sameasfull", ui->checkBoxSameAsFull->isChecked());

    w->setupFont();

    auto vlist = tts.availableVoices();
    if (ui->comboBoxVoice->count() > 0)
        w->settings->setValue("?voice/voice", vlist[ui->comboBoxVoice->currentIndex()].name());
    w->settings->setValue("?voice/volume", ui->horizontalSliderVolume->value() / 100.0);
    w->settings->setValue("?voice/rate", ui->horizontalSliderRate->value() / 100.0);
    w->settings->setValue("?voice/pitch", ui->horizontalSliderPitch->value() / 100.0);
    w->setupVoice();

    w->currentOutput->needRedraw = true;
    w->currentOutput->update();
}

void DialogConfig::on_pushButtonListen_clicked()
{
    auto vlist = w->textToSpeech.availableVoices();
    if (vlist.empty()) {
        QMessageBox::warning(this, tr("错误"), tr("没有可用的语音。"));
        return;
    }
    tts.setVoice(vlist[ui->comboBoxVoice->currentIndex()]);
    tts.setVolume(ui->horizontalSliderVolume->value() / 100.0);
    tts.setRate(ui->horizontalSliderRate->value() / 100.0);
    tts.setPitch(ui->horizontalSliderPitch->value() / 100.0);

    tts.say(tr("欢迎使用POPKC阅读软件"));
}

void DialogConfig::on_pushButtonFont_clicked()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, font, this);
    //qDebug(f.styleName().toUtf8());
    if (ok) {
        font = f;
    }
}

void DialogConfig::on_pushButtonFront_clicked()
{
    QColor c = QColorDialog::getColor(frontColor, this);
    if (c.isValid())
        frontColor = c;
}

void DialogConfig::on_pushButtonDefault_clicked()
{
    frontColor.setRgb(DEFAULTFONTR, DEFAULTFONTG, DEFAULTFONTB);
    backColor.setRgb(DEFAULTBGR, DEFAULTBGG, DEFAULTBGB);
    font.setFamily(DEFAULT_FONTFAMALY);
    font.setPointSize(DEFAULT_FONTPOINTSIZE);
    font.setStyleName(DEFAULT_FONTSTYLE);
    font.setWeight(DEFAULT_FONTPOINTWEIGHT);
    font.setItalic(DEFAULT_FONTITALIC);
    ui->spinBoxLineSpace->setValue(DEFAULT_LINESPACE);
    ui->spinBoxPadding->setValue(DEFAULT_PADDING);
    ui->horizontalSliderRollRate->setValue(DEFAULT_ROLLRATE);
    ui->checkBoxSmartReturn->setChecked(DEFAULT_SMARTRETURN);

    olFrontColor.setRgb(ONELINE_FONTR, ONELINE_FONTG, ONELINE_FONTB);
    olBackColor.setRgb(ONELINE_BGR, ONELINE_BGG, ONELINE_BGB);
    olFont.setFamily(ONELINE_FONTFAMALY);
    olFont.setPointSize(ONELINE_FONTPOINTSIZE);
    olFont.setStyleName(ONELINE_FONTSTYLE);
    olFont.setWeight(ONELINE_FONTPOINTWEIGHT);
    olFont.setItalic(ONELINE_FONTITALIC);
    ui->spinBoxOLLineSpace->setValue(ONELINE_LINESPACE);
    ui->spinBoxOLPadding->setValue(ONELINE_PADDING);
    ui->checkBoxSameAsFull->setChecked(ONELINE_SAMEASFULL);
}

void DialogConfig::on_pushButtonBack_clicked()
{
    QColor c = QColorDialog::getColor(backColor, this);
    if (c.isValid())
        backColor = c;
}

void DialogConfig::on_horizontalSliderVolume_valueChanged(int value)
{
    QString s = QString::number(value) + "%";
    ui->labelVolume->setText(s);
}

void DialogConfig::on_horizontalSliderRate_valueChanged(int value)
{
    QString s = QString::number(value) + "%";
    ui->labelRate->setText(s);
}

void DialogConfig::on_horizontalSliderPitch_valueChanged(int value)
{
    QString s = QString::number(value) + "%";
    ui->labelPitch->setText(s);
}

void DialogConfig::on_checkBoxSameAsFull_toggled(bool checked)
{
    if (checked) {
        ui->pushButtonOLBack->setEnabled(false);
        ui->pushButtonOLFont->setEnabled(false);
        ui->pushButtonOLFront->setEnabled(false);
        ui->spinBoxOLPadding->setEnabled(false);
        ui->spinBoxOLLineSpace->setEnabled(false);
    }
    else {
        ui->pushButtonOLBack->setEnabled(true);
        ui->pushButtonOLFont->setEnabled(true);
        ui->pushButtonOLFront->setEnabled(true);
        ui->spinBoxOLPadding->setEnabled(true);
        ui->spinBoxOLLineSpace->setEnabled(true);
    }
}

void DialogConfig::on_pushButtonOLFont_clicked()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, olFont, this);
    if (ok) {
        olFont = f;
    }
}

void DialogConfig::on_pushButtonOLFront_clicked()
{
    QColor c = QColorDialog::getColor(olFrontColor, this);
    if (c.isValid())
        olFrontColor = c;
}

void DialogConfig::on_pushButtonOLBack_clicked()
{
    QColor c = QColorDialog::getColor(olBackColor, this);
    if (c.isValid())
        olBackColor = c;
}
