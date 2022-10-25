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
#include "mainwindow.h"
#include "core/widgetoneline.h"
#include "dialog/dialogconfig.h"
#include "dialog/dialogindex.h"
#include "dialog/dialogsearch.h"
#include "dialog/formrollrate.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    dialogConfig = nullptr;
    dialogSearch = nullptr;
    dialogIndex = nullptr;
    stopWord = ",.?!:;<，。？！：;\r\n—…、";

    ui->setupUi(this);

    setWindowIcon(QIcon(":/icon.png"));
    waRollRate = new QWidgetAction(ui->menuJump);
    formRollRate = new FormRollRate();
    waRollRate->setDefaultWidget(formRollRate);
    ui->menuAutoRead->addAction(waRollRate);

    trayIcon = nullptr;

    /*auto clist=QTextCodec::availableCodecs();
    for(auto &b : clist) {
        qDebug("%s", b.constData());
    }*/
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;
}

void MainWindow::init()
{
    settings = new QSettings("./" + QApplication::applicationName() + ".ini", QSettings::IniFormat, this);
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("./" + QApplication::applicationName() + ".sqlite");
    if (db.open()) {
        db.exec("CREATE TABLE IF NOT EXISTS 'files' ("
                "'file'	TEXT NOT NULL UNIQUE,"
                "PRIMARY KEY('file')"
                ")");
        db.exec("CREATE TABLE IF NOT EXISTS 'index' ("
                "'file'	INTEGER NOT NULL,"
                "'pos'	INTEGER NOT NULL,"
                "'string'	TEXT NOT NULL,"
                "PRIMARY KEY('file','pos')"
                ") WITHOUT ROWID");
    }

    ui->widgetOneLine->init();
    ui->actionShowTray->setChecked(settings->value("?app/showtray", true).toBool());

    if (settings->value("?app/onelinemode", false).toBool())
        oneLineMode();
    else
        fullMode();

    paintInfo.fontm = nullptr;
    paintInfo.background.setStyle(Qt::SolidPattern);
    olPaintInfo.fontm = nullptr;
    olPaintInfo.background.setStyle(Qt::SolidPattern);
    setupFont();

    auto alist = ui->menuCodec->actions();
    for (QAction* ac : alist) {
        if (ac->isCheckable()) {
            connect(ac, SIGNAL(triggered(bool)), this, SLOT(actionCodecTriggered()));
        }
    }
    QByteArray ba = settings->value("?app/codec").toByteArray();
    if (!ba.isEmpty()) {
        for (QAction* ac : alist) {
            if (ac->isCheckable() && ac->text().toLocal8Bit() == ba) {
                codecTriggered(ac);
                goto acfound;
            }
        }
    }
    codecTriggered(ui->actionGBK);
acfound:;

    recentFiles = settings->value("?app/recentfiles").toStringList();
    resetRecentFiles();

    connect(&timerSave, &QTimer::timeout, this, &MainWindow::onTimerSave);
    timerSave.start(300000);

    if (!recentFiles.empty())
        fileInfo.loadFile(recentFiles.first());

#ifndef BOOKREADER_USE_QTSPEECH
    QTextToSpeech::init();
#endif
    setupVoice();

    timerRoll.setInterval(10);
    connect(&timerRoll, &QTimer::timeout, this, &MainWindow::onTimerRoll);
}

void MainWindow::init2()
{
    hotKeyRead = new QHotkey(QKeySequence(Qt::CTRL + Qt::Key_F12), true, QApplication::instance());
    if (hotKeyRead->isRegistered())
        connect(hotKeyRead, &QHotkey::activated, this, &MainWindow::handleHotKeyRead);
    else
        QMessageBox::warning(this, tr("错误"), tr("注册热键Ctrl+F12失败。"));
}

void MainWindow::oneLineMode()
{
    currentOutput = ui->widgetOneLine;
    ui->widgetOneLine->setVisible(true);
    ui->widgetFull->setVisible(false);
    ui->verticalScrollBar->setVisible(false);
    ui->actionShowTray->setEnabled(true);
    this->menuBar()->setVisible(false);
    isOneLine = true;
    currentOutput->needRedraw = true;
    currentOutput->offset = 0;
    ui->actionOneLineMode->setChecked(true);
    ui->actionFullMode->setChecked(false);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    if (ui->actionShowTray->isChecked()) {
        if (!trayIcon)
            createTrayIcon();
        trayIcon->show();
    }
}

void MainWindow::fullMode()
{
    currentOutput = ui->widgetFull;
    ui->widgetOneLine->setVisible(false);
    ui->widgetFull->setVisible(true);
    ui->verticalScrollBar->setVisible(true);
    ui->actionShowTray->setEnabled(false);
    this->menuBar()->setVisible(true);
    isOneLine = false;
    currentOutput->needRedraw = true;
    currentOutput->offset = 0;
    ui->actionOneLineMode->setChecked(false);
    ui->actionFullMode->setChecked(true);
    this->setWindowFlags(Qt::Window);
    if (trayIcon)
        trayIcon->hide();
}

void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon(":/icon.png"), this);
    actionShow = new QAction(tr("显示(&D)"), this);
    connect(actionShow, &QAction::triggered, this, &MainWindow::onActionShow);
    trayMenu = new QMenu(this);
    trayMenu->addAction(actionShow);
    trayMenu->addMenu(ui->menuConfig);
    trayMenu->addAction(ui->actionRead);
    trayMenu->addAction(ui->actionExit);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip(tr("POPKC阅读软件"));
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

/*int MainWindow::getRollRateStep()
{
    if(rollRate<20)
        return 1;
    else if(rollRate<50)
        return 2;
    else if(rollRate<100)
        return 5;
    else
        return 10;
}*/

void MainWindow::setupFont()
{
    int fontr, fontg, fontb, bgr, bgg, bgb;
    fontr = settings->value("?app/fontr", DEFAULTFONTR).toInt();
    fontg = settings->value("?app/fontg", DEFAULTFONTG).toInt();
    fontb = settings->value("?app/fontb", DEFAULTFONTB).toInt();
    bgr = settings->value("?app/bgr", DEFAULTBGR).toInt();
    bgg = settings->value("?app/bgg", DEFAULTBGG).toInt();
    bgb = settings->value("?app/bgb", DEFAULTBGB).toInt();
    paintInfo.font.setFamily(settings->value("?app/fontfamily", DEFAULT_FONTFAMALY).toString());
    paintInfo.font.setPointSize(settings->value("?app/fontsize", DEFAULT_FONTPOINTSIZE).toInt());
    paintInfo.font.setStyleName(settings->value("?app/fontstyle", DEFAULT_FONTSTYLE).toString());
    paintInfo.font.setWeight(settings->value("?app/fontweight", DEFAULT_FONTPOINTWEIGHT).toInt());
    paintInfo.font.setItalic(settings->value("?app/fontitalic", DEFAULT_FONTITALIC).toBool());

    if (paintInfo.fontm)
        delete paintInfo.fontm;
    paintInfo.fontm = new QFontMetrics(paintInfo.font);
    paintInfo.background.setColor(QColor(bgr, bgg, bgb));
    paintInfo.fontcolor.setColor(QColor(fontr, fontg, fontb));
    //paintInfo.selectedColor.setColor(QColor(255 - bgr, 255 - bgg, 255 - bgb));
    paintInfo.selectedColor.setColor(QColor(bgr < 128 ? 255 : 0, bgg < 128 ? 255 : 0, bgb < 128 ? 255 : 0));

    paintInfo.linespace = settings->value("?app/linespace", DEFAULT_LINESPACE).toUInt();
    paintInfo.padding = settings->value("?app/padding", DEFAULT_PADDING).toUInt();
    paintInfo.smartReturn = settings->value("?app/smartreturn", DEFAULT_SMARTRETURN).toBool();

    rollRate = settings->value("?app/rollrate", DEFAULT_ROLLRATE).toInt();
    if (rollRate <= MIN_ROLLRATE || rollRate > MAX_ROLLRATE)
        rollRate = DEFAULT_ROLLRATE;
    formRollRate->renewValue();

    if (settings->value("?oneline/sameasfull", ONELINE_SAMEASFULL).toBool()) {
        ui->widgetOneLine->pi = &paintInfo;
    }
    else {
        ui->widgetOneLine->pi = &olPaintInfo;
        fontr = settings->value("?oneline/fontr", ONELINE_FONTR).toInt();
        fontg = settings->value("?oneline/fontg", ONELINE_FONTG).toInt();
        fontb = settings->value("?oneline/fontb", ONELINE_FONTB).toInt();
        bgr = settings->value("?oneline/bgr", ONELINE_BGR).toInt();
        bgg = settings->value("?oneline/bgg", ONELINE_BGG).toInt();
        bgb = settings->value("?oneline/bgb", ONELINE_BGB).toInt();
        olPaintInfo.font.setFamily(settings->value("?oneline/fontfamily", ONELINE_FONTFAMALY).toString());
        olPaintInfo.font.setPointSize(settings->value("?oneline/fontsize", ONELINE_FONTPOINTSIZE).toInt());
        olPaintInfo.font.setStyleName(settings->value("?oneline/fontstyle", ONELINE_FONTSTYLE).toString());
        olPaintInfo.font.setWeight(settings->value("?oneline/fontweight", ONELINE_FONTPOINTWEIGHT).toInt());
        olPaintInfo.font.setItalic(settings->value("?oneline/fontitalic", ONELINE_FONTITALIC).toBool());

        if (olPaintInfo.fontm)
            delete olPaintInfo.fontm;
        olPaintInfo.fontm = new QFontMetrics(olPaintInfo.font);
        olPaintInfo.background.setColor(QColor(bgr, bgg, bgb));
        olPaintInfo.fontcolor.setColor(QColor(fontr, fontg, fontb));
        //olPaintInfo.selectedColor.setColor(QColor(255 - bgr, 255 - bgg, 255 - bgb));

        olPaintInfo.linespace = settings->value("?oneline/linespace", ONELINE_LINESPACE).toUInt();
        olPaintInfo.padding = settings->value("?oneline/padding", ONELINE_PADDING).toUInt();
    }

    if (isOneLine)
        static_cast<WidgetOneLine*>(currentOutput)->adjustHeight();
    currentOutput->changeCacheSize();
}

void MainWindow::setupVoice()
{
    auto vlist = textToSpeech.availableVoices();
    QString s = settings->value("?voice/voice").toString();
    QVoice voice;
    if (!s.isEmpty()) {
        for (const QVoice& v : vlist) {
            if (v.name() == s) {
                voice = v;
                goto voiceFound;
            }
        }
    }

    if (!vlist.isEmpty())
        voice = vlist.first();
    else
        return;
voiceFound:
    textToSpeech.setVoice(voice);
    textToSpeech.setVolume(settings->value("?voice/volume", DEFAULTVOLUME).toDouble());
    textToSpeech.setPitch(settings->value("?voice/pitch", DEFAULTPITCH).toDouble());
    textToSpeech.setRate(settings->value("?voice/rate", DEFAULTRATE).toDouble());
}

void MainWindow::resetRecentFiles()
{
    auto alist = ui->menuRecentFiles->actions();
    for (auto a : alist) {
        a->deleteLater();
    }

    for (const QString& s : recentFiles) {
        QAction* ac = new QAction(ui->menuRecentFiles);
        ac->setText(s);
        ui->menuRecentFiles->addAction(ac);
        connect(ac, SIGNAL(triggered(bool)), this, SLOT(onActionRecentFiles()));
    }
}

void MainWindow::codecTriggered(QAction* ac)
{
    auto alist = ui->menuCodec->actions();
    for (QAction* a : alist) {
        if (a->isCheckable())
            a->setChecked(false);
    }
    ac->setChecked(true);
    fileInfo.codec = QTextCodec::codecForName(ac->iconText().toLocal8Bit());
    //assert(fileInfo.codec);
    if (fileInfo.file.isOpen()) {
        settings->setValue(fileInfo.file.fileName() + "/?codec", fileInfo.codec->name());
        currentOutput->needRedraw = true;
        currentOutput->update();
    }
}

void MainWindow::actionOpenTriggered()
{
    QString s;
    if (fileInfo.file.isOpen()) {
        QFileInfo fi(fileInfo.file);
        s = fi.absolutePath();
    }
    s = QFileDialog::getOpenFileName(this, "", s, tr("文本文件 (*.txt);;所有文件 (*)"));
    if (!s.isEmpty()) {
        fileInfo.close();
        fileInfo.loadFile(s);
    }
}

void MainWindow::actionCodecTriggered()
{
    QAction* ac = static_cast<QAction*>(sender());
    if (ac->isChecked()) {
        codecTriggered(ac);
        settings->setValue("?app/codec", ac->text().toLocal8Bit());
    }
    else
        ac->setChecked(true);
}

void MainWindow::handleHotKeyRead()
{
    ui->actionRead->toggle();
}

void MainWindow::onTimerRoll()
{
    //qDebug(QString::number(remainRollTime).toUtf8());
    lldiv_t dt = ::lldiv(etRoll.restart() * rollRate + remainRollTime, 500);
    remainRollTime = dt.rem;
    if (dt.quot > 0)
        currentOutput->addOffset(dt.quot);
}

void MainWindow::closeEvent(QCloseEvent*)
{
    fileInfo.saveReadPos();
    settings->setValue("?app/rollrate", rollRate);
    currentOutput->saveState();
    if (trayIcon)
        trayIcon->hide();
    settings->sync();
    QApplication::quit();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_verticalScrollBar_actionTriggered(int action)
{
    if (fileInfo.file.isOpen() && !ui->actionRead->isChecked()) {
        switch (action) {
        case QAbstractSlider::SliderSingleStepAdd:
            currentOutput->lineMoveDown();
            break;
        case QAbstractSlider::SliderSingleStepSub:
            currentOutput->lineMoveUp();
            break;
        case QAbstractSlider::SliderPageStepAdd:
            currentOutput->pageMoveDown();
            break;
        case QAbstractSlider::SliderPageStepSub:
            currentOutput->pageMoveUp();
            break;
        case QAbstractSlider::SliderMove:
            currentOutput->randomMove(ui->verticalScrollBar->sliderPosition());
            break;
        default:
            break;
        }
    }
    ui->verticalScrollBar->setValue(w->fileInfo.currentPos);
}

void MainWindow::onTimerSave()
{
    fileInfo.saveReadPos();
    settings->sync();
}

void MainWindow::onActionRecentFiles()
{
    QAction* ac = static_cast<QAction*>(sender());
    w->fileInfo.close();
    w->fileInfo.loadFile(ac->text());
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        onActionShow();
    }
}

void MainWindow::onActionShow()
{
    WidgetOneLine* wol = static_cast<WidgetOneLine*>(currentOutput);
    wol->adjustPos();
    if (wol->hidding)
        wol->endHide();
}

void MainWindow::on_actionMoveNext_triggered()
{
    if (fileInfo.file.isOpen()) {
        if (fileInfo.currentPos < fileInfo.file.size() - 1) {
            fileInfo.currentPos++;
            currentOutput->needRedraw = true;
            currentOutput->update();
        }
    }
}

void MainWindow::on_actionMovePrev_triggered()
{
    if (fileInfo.file.isOpen()) {
        if (fileInfo.currentPos > 0) {
            fileInfo.currentPos--;
            currentOutput->needRedraw = true;
            currentOutput->update();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (fileInfo.file.isOpen()) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            ui->actionAutoRoll->toggle();
            return;
        }
        if (!ui->actionRead->isChecked()) {
            switch (event->key()) {
            case Qt::Key_PageDown:
                currentOutput->pageMoveDown();
                break;
            case Qt::Key_PageUp:
                currentOutput->pageMoveUp();
                break;
            case Qt::Key_Down:
                currentOutput->lineMoveDown();
                break;
            case Qt::Key_Up:
                currentOutput->lineMoveUp();
                break;
            default:
                goto nothandle;
            }
            ui->verticalScrollBar->setValue(fileInfo.currentPos);
            return;
        nothandle:
            if (ui->actionAutoRoll->isChecked()) {
                switch (event->key()) {
                case Qt::Key_Left:
                    if (rollRate > MIN_ROLLRATE) {
                        rollRate--;
                        formRollRate->renewValue();
                    }
                    return;
                case Qt::Key_Right:
                    if (rollRate < MAX_ROLLRATE) {
                        rollRate++;
                        formRollRate->renewValue();
                    }
                    return;
                default:
                    break;
                }
            }
        }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::on_actionConfig_triggered()
{
    if (!dialogConfig)
        dialogConfig = new DialogConfig(this);
    dialogConfig->init();
    if (dialogConfig->exec() == QDialog::Accepted) {
        dialogConfig->save();
    }
}

void MainWindow::on_actionAboutQt_triggered()
{
    QApplication::aboutQt();
}

void MainWindow::on_actionAutoRoll_toggled(bool checked)
{
    if (checked) {
        if (ui->actionRead->isChecked())
            ui->actionRead->setChecked(false);
        remainRollTime = 0;
        etRoll.start();
        timerRoll.start();
    }
    else {
        timerRoll.stop();
    }
}

void MainWindow::on_actionRead_toggled(bool checked)
{
    if (checked) {
        if (ui->actionAutoRoll->isChecked())
            ui->actionAutoRoll->setChecked(false);
        fileInfo.startReading();
    }
    else
        fileInfo.stopReading();
}

void MainWindow::on_actionSearch_triggered()
{
    if (!dialogSearch) {
        dialogSearch = new DialogSearch(this);
    }
    dialogSearch->exec();
}

void MainWindow::on_actionIndex_triggered()
{
    if (!dialogIndex) {
        dialogIndex = new DialogIndex(this);
        if (!w->db.isOpen()) {
            QMessageBox::warning(this, "错误", "sqlite数据库打开失败，无法保存和读取章节索引。");
        }
    }
    dialogIndex->init();
    dialogIndex->exec();
    dialogIndex->save();
}

void MainWindow::on_actionFullMode_triggered(bool checked)
{
    if (checked) {
        currentOutput->saveState();
        fullMode();
        currentOutput->restoreState();
        settings->setValue("?app/onelinemode", false);
    }
    else
        ui->actionFullMode->setChecked(true);
}

void MainWindow::on_actionOneLineMode_triggered(bool checked)
{
    if (checked) {
        currentOutput->saveState();
        oneLineMode();
        currentOutput->restoreState();
        static_cast<WidgetOneLine*>(currentOutput)->adjustHeight();
        settings->setValue("?app/onelinemode", true);
    }
    else
        ui->actionOneLineMode->setChecked(true);
}

void MainWindow::on_actionShowTray_triggered(bool checked)
{
    if (checked) {
        if (!trayIcon)
            createTrayIcon();
        trayIcon->show();
        settings->setValue("?app/showtray", true);
    }
    else {
        trayIcon->hide();
        settings->setValue("?app/showtray", false);
    }
}
