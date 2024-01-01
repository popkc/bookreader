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
#include "core/widgetoneline.h"
#include "mainwindow.h"
#include "pch.h"
#include "ui_mainwindow.h"

WidgetOneLine::WidgetOneLine(QWidget *parent)
    : WidgetOutput(parent)
{
    setMouseTracking(true);
    currentSpace = 0;
    cacheRealWidth = 0;
    hidePos = HIDEPOS_NONE;
    hidding = false;
}

WidgetOneLine::~WidgetOneLine()
{
}

void WidgetOneLine::changeCacheSize()
{
    cacheRealWidth = width() + pi->fontm->maxWidth() * 2;
    cache = QPixmap(cacheRealWidth + pi->fontm->maxWidth(), height());
    needRedraw = true;
    prevLine = prevPage = nullptr;
}

void WidgetOneLine::lineMoveDown()
{
    offset = 0;
    currentSpace = 0;
    if (!w->textsInfo.empty()) {
        prevLine = w->fileInfo.currentPos + w->fileInfo.content;
        prevPage = nullptr;
        auto it = w->textsInfo.begin();
        int x = it->screenPos.x();
        it++;
        while (it != w->textsInfo.end()) {
            int ix = it->screenPos.x();
            if (ix != x) {
                w->fileInfo.setCurrentPos(it->contentPos - w->fileInfo.content);
                int end = ix + width() - pi->padding * 2;
                if (end >= cacheRealWidth)
                    end -= cacheRealWidth;
                QPoint &sp = w->textsInfo.last().screenPos;
                int le = sp.x();
                if (sp.y() == -1)
                    le += pi->fontm->maxWidth() * 2;
                else
                    le += sp.y();
                if (le >= cacheRealWidth)
                    le -= cacheRealWidth;

                cacheStartPos = ix;
                w->textsInfo.erase(w->textsInfo.begin(), it);
                textRemain(le, end, ix);
                update();
                return;
            }
            it++;
        }
        w->fileInfo.setCurrentPos(getNextPos((--it)->contentPos) - w->fileInfo.content);
    }
    renewCache();
    update();
}

void WidgetOneLine::lineMoveUp()
{
    if (w->fileInfo.currentPos == 0) {
        if (offset || currentSpace) {
            offset = 0;
            currentSpace = 0;
            update();
        }
        return;
    }
    prevPage = nullptr;
    currentSpace = 0;
    offset = 0;
    if (prevLine) {
        w->fileInfo.setCurrentPos(prevLine - w->fileInfo.content);
        prevLine = nullptr;
    }
    else {
        prevLine = nullptr;
        char *p = w->fileInfo.currentPos + w->fileInfo.content;
        int ct = w->fileInfo.getCodecType();
        QString s;
        char *spaceStart;
        char *lp = p;
        int spacing = 0;
        for (;;) {
            p--;
            s = getLastWord(ct, p);
            if (!s.isEmpty()) {
                if (iswspace(s[0].unicode()) || s[0] == L'　') {
                    if (spacing == 0)
                        spaceStart = p;

                    if (spacing != 1) {
                        if (s[0] == '\n')
                            spacing = 1;
                        else
                            spacing = 2;
                    }
                }
                else {
                    if (spacing == 2) {
                        w->fileInfo.setCurrentPos(spaceStart - w->fileInfo.content);
                    }
                    else if (spacing == 1) {
                        w->fileInfo.setCurrentPos(lp - w->fileInfo.content);
                        prevLine = p;
                    }
                    else {
                        w->fileInfo.setCurrentPos(p - w->fileInfo.content);
                    }
                    break;
                }
            }
            else if (p <= w->fileInfo.content) {
                w->fileInfo.setCurrentPos(0);
                break;
            }
            lp = p;
        }
    }
    renewCache();
    update();
}

void WidgetOneLine::pageMoveDown()
{
    offset = 0;
    if (w->textsInfo.empty()) {
        int rw = width() - pi->padding * 2;
        if (currentSpace >= rw) {
            currentSpace -= rw;
            if (currentSpace >= rw)
                return;
        }
        else
            return;
    }
    else {
        char *p;
        char *pl;
        if (spaceRemain) {
            pl = w->textsInfo.last().contentPos;
            p = getNextPos(pl);
        }
        else {
            auto it = w->textsInfo.end() - 1;
            if (it != w->textsInfo.begin()) {
                p = it->contentPos;
                it--;
                pl = it->contentPos;
            }
            else {
                pl = it->contentPos;
                p = getNextPos(it->contentPos);
            }
        }

        if (p >= w->fileInfo.contentEnd)
            return;
        prevLine = pl;
        prevPage = w->fileInfo.content + w->fileInfo.currentPos;
        currentSpace = spaceRemain;
        w->fileInfo.setCurrentPos(p - w->fileInfo.content);
    }
    renewCache();
    update();
}

void WidgetOneLine::pageMoveUp()
{
    if (w->fileInfo.currentPos == 0) {
        if (offset || currentSpace) {
            offset = 0;
            currentSpace = 0;
            update();
        }
        return;
    }
    currentSpace = 0;
    offset = 0;
    prevLine = nullptr;
    if (prevPage)
        w->fileInfo.setCurrentPos(prevPage - w->fileInfo.content);
    else {
        int nw;
        nw = width() - pi->padding * 2;
        if (!w->textsInfo.empty()) {
            int y = w->textsInfo.first().screenPos.y();
            if (y == -1)
                nw -= pi->fontm->maxWidth() * 2;
            else
                nw -= y;
        }

        int piece = w->fileInfo.currentPos / PIECESIZE - 1;
        if (piece >= 0)
            w->fileInfo.loadPiece(piece);
        int codecType = w->fileInfo.getCodecType();

        QString s;
        int spacing = 0;
        char *spaceStart;
        QList<QPair<QChar, char *>> spaceInfos;
        char *p = w->fileInfo.content + w->fileInfo.currentPos;
        char *lp = p;
        for (;;) {
            p--;
            s = getLastWord(codecType, p);
            if (!s.isEmpty()) {
                if (iswspace(s[0].unicode()) || s[0] == L'　') {
                    if (spacing == 0) {
                        spaceStart = lp;
                        spaceInfos.clear();
                    }

                    if (spacing != 1) {
                        if (s[0] == '\n')
                            spacing = 1;
                        else {
                            spacing = 2;
                            spaceInfos.append(QPair<QChar, char *>(s[0], p));
                        }
                    }
                }
                else {
                    if (spacing) {
                        if (spacing == 1) {
                            nw -= pi->fontm->maxWidth() * 2;
                            if (nw <= 0) {
                                lp = spaceStart;
                                goto reach;
                            }
                        }
                        else {
                            auto it = spaceInfos.end();
                            do {
                                it--;
                                nw -= pi->fontm->width(it->first);
                                if (nw <= 0) {
                                    it++;
                                    if (it != spaceInfos.end())
                                        lp = it->second;
                                    goto reach;
                                }
                            } while (it != spaceInfos.begin());
                        }
                        spacing = 0;
                    }
                    nw -= pi->fontm->width(s[0]);
                    if (nw <= 0) {
                    reach:;
                        if (w->fileInfo.currentPos == lp - w->fileInfo.content)
                            w->fileInfo.setCurrentPos(p - w->fileInfo.content);
                        else {
                            w->fileInfo.setCurrentPos(lp - w->fileInfo.content);
                            prevLine = p;
                        }
                        break;
                    }
                }
            }
            else if (p <= w->fileInfo.content) {
                w->fileInfo.setCurrentPos(0);
                break;
            }
            lp = p;
        }
    }
    prevPage = nullptr;
    renewCache();
    update();
}

void WidgetOneLine::randomMove(quint32 pos)
{
    changeCurrentPos(pos);
}

void WidgetOneLine::renewCache()
{
    needRedraw = false;
    cacheStartPos = 0;
    w->textsInfo.clear();
    prepareCachePainter();
    painter.fillRect(cache.rect(), pi->background);
    if (!w->fileInfo.file.isOpen()) {
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, WELCOMETEXT);
    }
    else {
        int wid = width() - pi->padding * 2;
        if (wid > currentSpace && w->fileInfo.currentPos < w->fileInfo.file.size()) {
            textCache(currentSpace, wid - currentSpace, w->fileInfo.content + w->fileInfo.currentPos);
        }
    }

    painter.end();
}

void WidgetOneLine::addOffset(int value)
{
    offset += value;
    if (currentSpace) {
        if (offset <= currentSpace) {
            currentSpace -= offset;
            cacheStartPos -= offset;
            offset = 0;
        }
        else {
            cacheStartPos -= currentSpace;
            offset -= currentSpace;
            currentSpace = 0;
        }
    }

    if (cacheStartPos >= cacheRealWidth)
        cacheStartPos -= cacheRealWidth;

start:;
    if (offset) {
        auto it = w->textsInfo.begin();
        while (it != w->textsInfo.end()) {
            int y = it->screenPos.y();
            if (y != 0) {
                if (y == -1)
                    y = pi->fontm->maxWidth() * 2;

                if (y <= offset) {
                    int oo = offset - y;
                    quint32 cp = w->fileInfo.currentPos;
                    lineMoveDown();
                    if (cp == w->fileInfo.currentPos) {
                        w->ui->actionAutoRoll->setChecked(false);
                        return;
                    }
                    offset = oo;
                    goto start;
                }
                else
                    break;
            }
            it++;
        }
    }

    if (w->textsInfo.empty())
        renewCache();
    else {
        int sp = cacheStartPos + offset;
        if (sp >= cacheRealWidth)
            sp -= cacheRealWidth;
        int end = sp + width() - pi->padding * 2;
        if (end >= cacheRealWidth)
            end -= cacheRealWidth;

        int y = w->textsInfo.last().screenPos.y();
        int le = w->textsInfo.last().screenPos.x();
        if (y == -1)
            y = pi->fontm->maxWidth() * 2;
        le += y;
        if (le >= cacheRealWidth)
            le -= cacheRealWidth;

        textRemain(le, end, sp);
    }
    update();
}

void WidgetOneLine::adjustHeight()
{
    w->resize(w->width(), pi->fontm->height() + pi->linespace * 2);
}

void WidgetOneLine::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    pressMousePos = event->globalPos();
    pressRect = w->geometry();
    if (event->pos().x() >= width() - 3)
        pressState = PressRight;
    else
        pressState = PressMiddle;
}

void WidgetOneLine::mouseMoveEvent(QMouseEvent *event)
{

    if (hidding) {
        endHide();
    }

    if (event->buttons() & Qt::LeftButton) {
        if (pressState == PressRight) {
            int wid = pressRect.width() + event->globalX() - pressMousePos.x();
            if (wid < 10)
                wid = 10;
            w->resize(wid, w->height());
        }
        else {
            QPoint p = pressRect.topLeft();
            p.rx() += event->globalX() - pressMousePos.x();
            p.ry() += event->globalY() - pressMousePos.y();
            w->move(p);
        }
    }
    else {
#ifdef _WIN32
        if (GetForegroundWindow() != (HWND)w->winId()) {
            // qDebug("gfw!");
            INPUT ip[2] = { 0 };
            ip[0].type = INPUT_MOUSE;
            ip[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ip[1].type = INPUT_MOUSE;
            ip[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(2, ip, sizeof(INPUT));
        }
#endif // _WIN32

        if (event->pos().x() >= width() - 3)
            setCursor(Qt::SizeHorCursor);
        else
            setCursor(Qt::ArrowCursor);
    }
}

void WidgetOneLine::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || pressState != PressMiddle)
        return;

    adjustPos();
}

void WidgetOneLine::paintEvent(QPaintEvent *event)
{
    if (needRedraw) {
        renewCache();
    }

    painter.begin(this);
    if (w->fileInfo.file.isOpen()) {
        QRect re = event->rect();
        if (re.left() < (int)pi->padding) {
            painter.fillRect(re.left(), re.top(), pi->padding - re.left(), re.height(), pi->background);
            re.setLeft(pi->padding);
        }

        if (re.right() >= width() - (int)pi->padding) {
            painter.fillRect(width() - pi->padding, re.top(), re.right() - width() + pi->padding + 1, re.height(), pi->background);
            re.setRight(width() - pi->padding - 1);
        }

        int sp = cacheStartPos + offset;
        if (sp >= cacheRealWidth)
            sp -= cacheRealWidth;
        int ep = sp + width() - pi->padding * 2;
        if (ep > sp) {
            if (ep >= cacheRealWidth) {
                int fl = cacheRealWidth - sp;
                if (re.left() - (int)pi->padding < fl) {
                    if (re.width() < fl)
                        fl = re.width();
                    painter.drawPixmap(re.left(), re.top(), cache, re.left() - pi->padding + sp, re.top(), fl, re.height());
                    if (re.width() != fl) {
                        re.setLeft(re.left() + fl);
                        painter.drawPixmap(re.left(), re.top(), cache, 0, re.top(), re.width(), re.height());
                    }
                }
                else {
                    painter.drawPixmap(re.left(), re.top(), cache, re.left() - pi->padding - fl, re.top(), re.width(), re.height());
                }
            }
            else {
                painter.drawPixmap(re.left(), re.top(), cache, re.left() - pi->padding + sp, re.top(), re.width(), re.height());
            }
        }
    }
    else {
        painter.drawPixmap(event->rect().topLeft(), cache, event->rect());
    }
    painter.end();
}

void WidgetOneLine::contextMenuEvent(QContextMenuEvent *event)
{
    w->popupMenu.popup(event->globalPos());
}

void WidgetOneLine::leaveEvent(QEvent *)
{
    if (hidePos != HIDEPOS_NONE && !w->popupMenu.isVisible() && !hidding) {
        hidding = true;
        switch (hidePos) {
        case WidgetOneLine::HIDEPOS_LEFT:
            w->move(1 - w->width(), w->y());
            break;
        case WidgetOneLine::HIDEPOS_RIGHT: {
            // QRect re = QApplication::desktop()->screenGeometry();
            auto re = QGuiApplication::screens()[0]->geometry();
            w->move(re.right(), w->y());
        } break;
        case WidgetOneLine::HIDEPOS_TOP:
            w->move(w->x(), 1 - w->height());
            break;
        case WidgetOneLine::HIDEPOS_BOTTOM: {
            // QRect re = QApplication::desktop()->screenGeometry();
            auto re = QGuiApplication::screens()[0]->geometry();
            w->move(w->x(), re.bottom());
        } break;
        default:
            break;
        }
    }
}

void WidgetOneLine::prepareCachePainter()
{
    painter.begin(&cache);
    painter.setPen(pi->fontcolor);
    painter.setFont(pi->font);
}

void WidgetOneLine::textCache(int x, int length, char *cpos)
{
    w->fileInfo.checkCurrentPiece();
    if (length < 0)
        length = 0;
    int end = x + length;
    bool newl;
    if (end > cacheRealWidth) {
        end -= cacheRealWidth;
        newl = true;
    }
    else
        newl = false;

    int y = pi->linespace + pi->fontm->ascent();
    QString s;
    QTextDecoder td(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    int spacing = 0;
    char *lp = cpos;
    int spaceStart;
    while (cpos < w->fileInfo.contentEnd) {
        s = td.toUnicode(cpos, 1);
        if (!s.isEmpty()) {
            w->textsInfo.append(TextInfo(s[0], x, -1, lp));
            lp = cpos + 1;
            if (iswspace(s[0].unicode()) || s[0] == L'　') {
                if (spacing == 0)
                    spaceStart = w->textsInfo.count() - 1;

                if (spacing != 1) {
                    if (s[0] == '\n')
                        spacing = 1;
                    else
                        spacing = 2;
                }
            }
            else {
                if (spacing) {
                    if (spacing == 1)
                        x += pi->fontm->maxWidth() * 2;
                    else {
                        for (int i = spaceStart; i < w->textsInfo.count() - 1; i++) {
                            int wid = pi->fontm->width(w->textsInfo[i].c);
                            w->textsInfo[i].screenPos.rx() = x;
                            w->textsInfo[i].screenPos.ry() = wid;
                            x += wid;
                        }
                    }

                    if (newl) {
                        if (x >= cacheRealWidth) {
                            x -= cacheRealWidth;
                            newl = false;
                        }
                    }
                    else if (x >= end) {
                        spaceRemain = x - end;
                        w->textsInfo.removeLast();
                        break;
                    }
                    spacing = 0;
                    w->textsInfo.last().screenPos.rx() = x;
                }

                painter.drawText(x, y, s);
                int wid = pi->fontm->width(s[0]);
                x += wid;
                w->textsInfo.last().screenPos.ry() = wid;

                if (x >= cacheRealWidth) {
                    int nx = x - cacheRealWidth;
                    if (newl) {
                        x = nx;
                        newl = false;
                    }

                    if (nx != 0) {
                        painter.drawPixmap(0, 0, cache, cacheRealWidth, 0, nx, cache.height());
                        painter.fillRect(cacheRealWidth, 0, nx, cache.height(), pi->background);
                    }
                }

                if (!newl && x >= end) {
                    spaceRemain = 0;
                    break;
                }
            }
        }
        cpos++;
    }
}

QString WidgetOneLine::getLastWord(int codecType, char *&p)
{
    QString s;
    QTextDecoder td(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    if (codecType == 0) { // utf8
        char *me = nullptr;
        while (p >= w->fileInfo.content) {
            if (static_cast<unsigned char>(*p) < 0x80) {
                s = td.toUnicode(p, 1);
                break;
            }
            else if (static_cast<unsigned char>(*p) < 0xc0) {
                if (me)
                    s = td.toUnicode(p, me - p + 1);
                else
                    s = td.toUnicode(p, 1);
                break;
            }
            else if (static_cast<unsigned char>(*p) < 0xfe) {
                if (!me)
                    me = p;
            }
            p--;
        }
    }
    else if (codecType == 1) { // utf16
        if (reinterpret_cast<uintptr_t>(p) & 1)
            p--;
        if (p >= w->fileInfo.content)
            s = td.toUnicode(p, 2);
    }
    else {
        if (p >= w->fileInfo.content) {
            if (static_cast<unsigned char>(*p) < 0x80)
                s = td.toUnicode(p, 1);
            else if (--p >= w->fileInfo.content)
                s = td.toUnicode(p, 2);
        }
    }
    return s;
}

int WidgetOneLine::cacheAbsDistance(int start, int end)
{
    if (start <= end)
        return end - start;
    else {
        // assert(end + cacheRealWidth - start >= 0);
        return end + cacheRealWidth - start;
    }
}

void WidgetOneLine::textRemain(int start, int end, int cacheStart)
{
    if (start == end)
        return;
    int cd = cacheAbsDistance(start, end);
    if (cd < cacheAbsDistance(start, cacheStart)) {
        prepareCachePainter();
        end += pi->fontm->maxWidth() * 2;
        if (end >= cacheRealWidth)
            end -= cacheRealWidth;
        int sle = start;
        if (start > end) {
            painter.fillRect(start, 0, cacheRealWidth - start, cache.height(), pi->background);
            start = 0;
        }
        painter.fillRect(start, 0, end - start, cache.height(), pi->background);
        textCache(sle, cd, getNextPos(w->textsInfo.last().contentPos));
        painter.end();
    }
}

void WidgetOneLine::adjustPos()
{
    // auto r = QApplication::desktop()->screenGeometry();
    auto r = QGuiApplication::screens()[0]->geometry();
    auto oor = w->geometry();
    if (w->width() > r.width())
        w->resize(r.width(), w->height());

    hidePos = HIDEPOS_NONE;
    if (oor.left() < 0) {
        w->move(0, w->y());
        hidePos = HIDEPOS_LEFT;
    }
    else if (oor.right() > r.right()) {
        w->move(r.right() - w->width() + 1, w->y());
        hidePos = HIDEPOS_RIGHT;
    }

    if (oor.top() < 0) {
        w->move(w->x(), 0);
        hidePos = HIDEPOS_TOP;
    }
    else if (oor.bottom() > r.bottom()) {
        w->move(w->x(), r.bottom() - w->height() + 1);
        hidePos = HIDEPOS_BOTTOM;
    }
}

void WidgetOneLine::endHide()
{
    hidding = false;
    switch (hidePos) {
    case WidgetOneLine::HIDEPOS_LEFT:
        w->move(0, w->y());
        break;
    case WidgetOneLine::HIDEPOS_RIGHT: {
        // QRect re = QApplication::desktop()->screenGeometry();
        auto re = QGuiApplication::screens()[0]->geometry();
        w->move(re.width() - w->width(), w->y());
    } break;
    case WidgetOneLine::HIDEPOS_TOP:
        w->move(w->x(), 0);
        break;
    case WidgetOneLine::HIDEPOS_BOTTOM: {
        // QRect re = QApplication::desktop()->screenGeometry();
        auto re = QGuiApplication::screens()[0]->geometry();
        w->move(w->x(), re.height() - w->height());
    } break;
    default:
        break;
    }
}
