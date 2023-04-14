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
#ifndef DIALOGINDEX_H
#define DIALOGINDEX_H

#include "pch.h"

namespace Ui {
class DialogIndex;
}

class DialogIndexAdvance;

#define DEFAULT_REGEXPS (QStringList() << tr("(^|[\\s　])(正文|作品相关|前传|后传|外传|引子|锲子)([\\s　]|$)") << tr("(^|[\\s　纪传])第?(\\d+|[两零一二三四五六七八九壹贰叁肆伍陆柒捌玖十百千万拾佰仟〇]+|序)([篇章节集卷部\\s　\\.\\-—]|$)") << tr("(^|[\\s　])[篇章节卷部集]\\s*(\\d|[两零一二三四五六七八九壹贰叁肆伍陆柒捌玖十百千万拾佰仟〇])") << tr("(纪|传)([\\s\\(（　]|$)"))
#define DEFAULT_MAXWORD 50
#define DEFAULT_QUZA true

class DialogIndex : public QDialog
{
    Q_OBJECT

public:
    explicit DialogIndex(QWidget* parent = 0);
    ~DialogIndex();
    void setupRegexps();
    void workIndex();
    void workLoad();
    void init();
    void save();
    QVector<QRegularExpression> regexps, regexps2;
    DialogIndexAdvance* dialogIndexAdvance;
    int maxWord;
    // std::thread threadIndex, threadLoad;
    std::atomic_bool running;
    quint32 startPos;
    std::atomic<int> finishedCount;
signals:
    void indexFound(char* pos, const QString& s, const QStringList& mlist);
    void process(int value);
private slots:
    void on_pushButtonAdvance_clicked();

    void on_pushButtonCreateIndex_clicked();

    void onIndexFound(char* pos, const QString& s, const QStringList& mlist);
    void onProcess(int value);
    void on_pushButtonClear_clicked();

    void on_pushButtonDel_clicked();

    void on_tableWidget_cellDoubleClicked(int row, int column);

    void on_pushButtonCurrentPos_clicked();

private:
    Ui::DialogIndex* ui;
    std::map<QStringList, int> mapMlist;
    QList<QStringList> mlistlist;
    int fileId;
    bool utf16;
    int lastPiece;
    int piece;
    QByteArray utf16n;
    bool changed;
    int isContinue;
    char* findNextN(char* pos);
    void discon();
    void selectCurrentPos();
    void testMlist(const QStringList& mlist);
};

#endif // DIALOGINDEX_H
