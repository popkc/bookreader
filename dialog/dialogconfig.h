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
#ifndef DIALOGCONFIG_H
#define DIALOGCONFIG_H

#include "pch.h"

#ifndef BOOKREADER_USE_QTSPEECH
#    include "core/texttospeech.h"
using namespace popkc;
#endif

namespace Ui {
class DialogConfig;
}

#define DEFAULTVOLUME 0.5
#define DEFAULTRATE 0.0
#define DEFAULTPITCH 0.0

#define DEFAULTFONTR 213
#define DEFAULTFONTG 238
#define DEFAULTFONTB 138
#define DEFAULTBGR 28
#define DEFAULTBGG 74
#define DEFAULTBGB 0

#define DEFAULT_FONTPOINTSIZE 30
#define DEFAULT_FONTPOINTWEIGHT QFont::Normal
#define DEFAULT_FONTITALIC false
#define DEFAULT_FONTSTYLE ""

#define DEFAULT_LINESPACE 0
#define DEFAULT_PADDING 1
#define DEFAULT_SMARTRETURN false

#define ONELINE_SAMEASFULL false
#define ONELINE_FONTR 213
#define ONELINE_FONTG 238
#define ONELINE_FONTB 138
#define ONELINE_BGR 28
#define ONELINE_BGG 74
#define ONELINE_BGB 0
#define ONELINE_FONTPOINTSIZE 24
#define ONELINE_FONTPOINTWEIGHT QFont::Normal
#define ONELINE_FONTITALIC false
#define ONELINE_FONTSTYLE ""
#define ONELINE_LINESPACE 0
#define ONELINE_PADDING 0

#define DEFAULT_ROLLRATE 26
#define MIN_ROLLRATE 1
#define MAX_ROLLRATE 150

#define DEFAULT_DISPLAY 0

#define POPKC_TITLE QObject::tr("POPKC阅读软件")

#ifdef _WIN32
#    define DEFAULT_FONTFAMALY "微软雅黑"
#else
#    define DEFAULT_FONTFAMALY "Noto Sans CJK SC"
#endif

#define ONELINE_FONTFAMALY DEFAULT_FONTFAMALY

class DialogConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DialogConfig(QWidget *parent = 0);
    ~DialogConfig();
    void init();
    void save();
    QTextToSpeech tts;
    QFont font, olFont;
    QColor frontColor, backColor, olFrontColor, olBackColor;
private slots:
    void on_pushButtonListen_clicked();

    void on_pushButtonFont_clicked();

    void on_pushButtonFront_clicked();

    void on_pushButtonDefault_clicked();

    void on_pushButtonBack_clicked();

    void on_horizontalSliderVolume_valueChanged(int value);

    void on_horizontalSliderRate_valueChanged(int value);

    void on_horizontalSliderPitch_valueChanged(int value);

    void on_checkBoxSameAsFull_toggled(bool checked);

    void on_pushButtonOLFont_clicked();

    void on_pushButtonOLFront_clicked();

    void on_pushButtonOLBack_clicked();

private:
    Ui::DialogConfig *ui;
};

#endif // DIALOGCONFIG_H
