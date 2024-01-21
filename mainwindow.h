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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/fileinfo.h"

#ifndef BOOKREADER_USE_QTSPEECH
#    include "core/texttospeech.h"
using namespace popkc;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
#    define HORIZONTALADVANCE horizontalAdvance
#else
#    define HORIZONTALADVANCE width
#endif

namespace Ui {
class MainWindow;
}

class WidgetOutput;
class DialogConfig;
class DialogSearch;
class DialogIndex;
class FormRollRate;
class DialogAbout;

struct PaintInfo
{
    QFont font;
    QPen fontcolor;
    QBrush background;
    QPen selectedColor;
    QFontMetrics *fontm;
    quint32 linespace;
    quint32 padding;
    bool smartReturn;
};

#define WELCOMETEXT QObject::tr("欢迎使用POPKC阅读软件！")

struct TextInfo
{
    TextInfo(const QChar &c, int x, int y, char *cpos)
        : c(c)
        , screenPos(x, y)
        , contentPos(cpos)
    {
    }
    QChar c;
    QPoint screenPos;
    char *contentPos;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void init();
    void init2();
    void codecTriggered(QAction *ac);
    void resetRecentFiles();
    void setupFont();
    void setupVoice();

    Ui::MainWindow *ui;

    QSqlDatabase db;
    PaintInfo paintInfo, olPaintInfo;
    QStringList recentFiles;
    QTextToSpeech textToSpeech;
    QString stopWord;
    FileInfo fileInfo;
    WidgetOutput *currentOutput;
    QSettings *settings;
    QSystemTrayIcon *trayIcon;
    QList<TextInfo> textsInfo;
    QAction *actionCopy;
    QMenu popupMenu;
    int rollRate;
    bool isOneLine;
    bool inited;
public slots:
    void actionOpenTriggered();
    void actionCodecTriggered();
    void handleHotKeyRead();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void on_actionExit_triggered();

    void on_verticalScrollBar_actionTriggered(int action);
    void onTimerSave();
    void onTimerRoll();
    void onActionRecentFiles();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onActionShow();
    void on_actionMoveNext_triggered();

    void on_actionMovePrev_triggered();

    void on_actionConfig_triggered();

    void on_actionAboutQt_triggered();

    void on_actionAutoRoll_toggled(bool checked);

    void on_actionRead_toggled(bool checked);

    void on_actionSearch_triggered();

    void on_actionIndex_triggered();

    void on_actionFullMode_triggered(bool checked);

    void on_actionOneLineMode_triggered(bool checked);

    void on_actionShowTray_triggered(bool checked);

    void on_actionCodecAutoDetect_triggered();

    void on_actionFullScreen_triggered();

    void on_actionNoBorder_triggered();

    void on_actionWindowed_triggered();

    void on_actionAbout_triggered();

private:
    void oneLineMode();
    void fullMode();
    void createTrayIcon();
    void dbInit();
    void onActionCopy();
    bool clearDisplayActions(QAction *ac);
    void myMaximized();
    // int getRollRateStep();

    QTimer timerSave;
    QTimer timerRoll;
    qint64 remainRollTime;
    QElapsedTimer etRoll;
    DialogConfig *dialogConfig;
    QHotkey *hotKeyRead;
    DialogSearch *dialogSearch;
    DialogIndex *dialogIndex;
    FormRollRate *formRollRate;
    QWidgetAction *waRollRate;
    QMenu *trayMenu;
    QAction *actionShow;
    DialogAbout *dialogAbout;
};

extern MainWindow *w;

#endif // MAINWINDOW_H
