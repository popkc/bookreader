﻿/*
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
#pragma once

#include "pch.h"

#ifndef BOOKREADER_USE_QTSPEECH
#    include "core/texttospeech.h"
using namespace popkc;
#endif

#define PIECESIZE 65536

class FileInfo : public QObject
{
    Q_OBJECT
public:
    FileInfo(QObject *parent = nullptr);
    ~FileInfo();

    void loadFile(const QString &fn);
    void close();
    void loadPiece(uintptr_t piece);
    void detectCodec();
    void checkCurrentPiece();
    void saveReadPos();
    void startReading();
    void stopReading();
    void prepareSentence();
    void setTitle();
    void setCurrentPos(uintptr_t npos);
    void renewMapIndex();
    void iterateDirectory(bool isNext);

    char *findLastParaStart(char *cpos);
    int getCodecType();

    QFile file;
    QMap<int, QString> mapIndex;
    std::atomic<bool> *pieceLoaded;
    uintptr_t pieceLoadedSize;
    char *content;
    char *contentEnd;
    uintptr_t currentPos;
    QTextCodec *codec;
    char *nextSentencePos;
    uint32_t sliderShift;
    bool haveRead;
    QMutex mutexFile;
public slots:
    void handleStateChanged(QTextToSpeech::State state);

private:
    int utf8Detect();
    void moveToNsp();
};

char *myMemrchr(intptr_t len, char *memEnd, char c);
