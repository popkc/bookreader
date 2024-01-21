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
#ifndef PCH_H
#define PCH_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include <algorithm>
#include <atomic>
#include <set>
#include <thread>
#include <utility>

#include <QtCore>

#ifdef BOOKREADER_USE_QTSPEECH
#    include <QTextToSpeech>
#else
#    include <sphelper.h>
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#    include <QtCore5Compat>
#endif

#include <QtGui>
#include <QtSql>
#include <QtWidgets>

/*
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
*/

#include <QHotkey>

#ifdef _MSC_VER
#    pragma execution_character_set("utf-8")
#endif

#endif // PCH_H
