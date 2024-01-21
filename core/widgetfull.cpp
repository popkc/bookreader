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
#include "core/widgetfull.h"
#include "mainwindow.h"
#include "pch.h"
#include "ui_mainwindow.h"

WidgetFull::WidgetFull(QWidget *parent)
    : WidgetOutput(parent)
{
    selectedStart = nullptr;
    selectedEnd = nullptr;
    dragging = false;

    QAction *ac = new QAction(this);
    ac->setText(tr("复制"));
    connect(ac, &QAction::triggered, this, &WidgetFull::onActionCopy);
    popupMenu = new QMenu(this);
    popupMenu->addAction(ac);
}

WidgetFull::~WidgetFull()
{
}

void WidgetFull::changeCacheSize()
{
    lineCount = height() / (w->paintInfo.linespace + w->paintInfo.fontm->height()) + 2;
    cache = QPixmap(width() - w->paintInfo.padding * 2, lineCount * w->paintInfo.fontm->height());
    needRedraw = true;
    prevLine = prevPage = nullptr;
}

void WidgetFull::lineMoveDown()
{
    offset = 0;
    if (!w->textsInfo.empty()) {
        auto it = w->textsInfo.begin();
        int y = it->screenPos.y();
        it++;
        while (it != w->textsInfo.end()) {
            if (it->screenPos.y() != y) {
                prevPage = nullptr;
                prevLine = w->fileInfo.currentPos + w->fileInfo.content;
                w->fileInfo.setCurrentPos(it->contentPos - w->fileInfo.content);
                if (needRedraw) {
                    renewCache();
                }
                else {
                    w->textsInfo.erase(w->textsInfo.begin(), it);
                    cacheStartPos++;
                    if (cacheStartPos == lineCount)
                        cacheStartPos = 0;
                    char *np = getNextPos(w->textsInfo.back().contentPos);
                    int nl;
                    nl = cacheStartPos + actualLines();

                    if (nl >= lineCount)
                        nl -= lineCount;

                    prepareCachePainter();
                    painter.fillRect(0, nl * w->paintInfo.fontm->height(), cache.width(),
                        w->paintInfo.fontm->height(), w->paintInfo.background);
                    textCache(nl, 1, np);
                    painter.end();
                }
                update();
                break;
            }
            it++;
        }
    }
}

void WidgetFull::lineMoveUp()
{
    if (w->fileInfo.currentPos == 0) {
        if (offset != 0) {
            offset = 0;
            update();
        }
        return;
    }
    offset = 0;
    char *newp;
    prevPage = nullptr;
    if (prevLine) {
        newp = prevLine;
        prevLine = nullptr;
    }
    else {
        prevLine = nullptr;
        char *cp = w->fileInfo.content + w->fileInfo.currentPos;
        char *p = w->fileInfo.findLastParaStart(cp);
        if (p) {
            auto lineStop = findLineStop(p, cp);

            if (lineStop.empty())
                newp = p;
            else {
                auto it = lineStop.end() - 1;
                newp = *it;
                if (it != lineStop.begin())
                    prevLine = *(--it);
                else
                    prevLine = p;
            }
        }
        else {
            newp = fillLine(cp);
        }
        assert(cp != newp);
    }

    w->fileInfo.setCurrentPos(newp - w->fileInfo.content);

    if (needRedraw || lineCount <= 2) {
        renewCache();
        update();
        return;
    }

    int k = actualLines();
    while (k >= lineCount - 1) {
        auto it = w->textsInfo.end() - 1;
        if (it != w->textsInfo.begin()) {
            int y = it->screenPos.y();
            it--;
            while (it != w->textsInfo.begin()) {
                if (it->screenPos.y() != y) {
                    it++;
                    w->textsInfo.erase(it, w->textsInfo.end());
                    break;
                }
                it--;
            }
        }
        k--;
    }

    cacheStartPos--;
    if (cacheStartPos < 0)
        cacheStartPos += lineCount;
    auto oti = std::move(w->textsInfo);
    prepareCachePainter();
    painter.fillRect(0, cacheStartPos * w->paintInfo.fontm->height(), cache.width(), w->paintInfo.fontm->height(),
        w->paintInfo.background);

    textCache(cacheStartPos, 1, newp);
    painter.end();
    w->textsInfo.append(oti);
    update();
}

void WidgetFull::pageMoveDown()
{
    offset = 0;
    auto it = w->textsInfo.end();
    if (it != w->textsInfo.begin()) {
        it--;
        if (lineCount <= 2) {
            char *npos = getNextPos(w->textsInfo.back().contentPos);
            if (npos < w->fileInfo.contentEnd) {
                prevLine = prevPage = w->fileInfo.content + w->fileInfo.currentPos;
                w->fileInfo.setCurrentPos(npos - w->fileInfo.content);
                renewCache();
            }
            else
                return;
        }
        else {
            int y = it->screenPos.y();
            while (it != w->textsInfo.begin()) {
                it--;
                if (y != it->screenPos.y()) {
                    auto nit = it;
                    it++;
                    prevPage = w->fileInfo.currentPos + w->fileInfo.content;
                    int y2 = nit->screenPos.y();
                    while (nit != w->textsInfo.begin()) {
                        nit--;
                        if (nit->screenPos.y() != y2) {
                            nit++;
                            break;
                        }
                    }
                    prevLine = nit->contentPos;

                    w->textsInfo.erase(w->textsInfo.begin(), it);
                    w->fileInfo.setCurrentPos(it->contentPos - w->fileInfo.content);
                    if (needRedraw) {
                        renewCache();
                    }
                    else {
                        int line = (y + w->paintInfo.fontm->descent()) / w->paintInfo.fontm->height();
                        cacheStartPos = line - 1;
                        if (line == lineCount)
                            line = 0;
                        int fe = line + lineCount - 2;

                        prepareCachePainter();
                        if (fe > lineCount) {
                            QRect re(QPoint(0, line * w->paintInfo.fontm->height()), cache.rect().bottomRight());
                            painter.fillRect(re, w->paintInfo.background);
                            painter.fillRect(0, 0, cache.width(), (fe - lineCount) * w->paintInfo.fontm->height(),
                                w->paintInfo.background);
                        }
                        else
                            painter.fillRect(0, line * w->paintInfo.fontm->height(), cache.width(),
                                fe * w->paintInfo.fontm->height(), w->paintInfo.background);

                        char *np = getNextPos(w->textsInfo.back().contentPos);
                        textCache(line, lineCount - 2, np);
                        painter.end();
                    }
                    break;
                }
            }
        }
        update();
    }
}

void WidgetFull::pageMoveUp()
{
    if (w->fileInfo.currentPos == 0) {
        if (offset != 0) {
            offset = 0;
            update();
        }
        return;
    }
    offset = 0;

    char *newp;
    prevLine = nullptr;
    if (prevPage) {
        newp = prevPage;
        prevPage = nullptr;
    }
    else {
        prevPage = nullptr;
        char *lp = w->fileInfo.content + w->fileInfo.currentPos;
        int needLine = lineCount - 2;
        if (needLine <= 0)
            needLine = 1;

        while ((newp = w->fileInfo.findLastParaStart(lp))) {
            auto ls = findLineStop(newp, lp);
            lp = newp;
            needLine -= ls.size() + 1;
            if (needLine == 0) {
                break;
            }
            else if (needLine < 0) {
                auto it = ls.begin();
                // 此时needLine用于表示lineStop中多出的行，不再是需要的行的意思
                it -= needLine + 1;
                if (it == ls.begin())
                    prevLine = newp;
                else
                    prevLine = *(it - 1);
                // 如果lineStop中多出的行超过一页的数量，则可以设置prevPage
                needLine += lineCount - 2;
                if (needLine == 0)
                    prevPage = newp;
                else if (needLine < 0)
                    prevPage = *(it - (lineCount - 2));

                newp = *it;
                break;
            }
            else if (newp == w->fileInfo.content)
                break;
        }

        if (!newp) {
            newp = lp;
            do {
                newp = fillLine(newp);
                if (newp == w->fileInfo.content)
                    break;
                needLine--;
            } while (needLine > 0);
        }
    }

    w->fileInfo.setCurrentPos(newp - w->fileInfo.content);
    renewCache();
    update();
}

void WidgetFull::randomMove(uintptr_t pos)
{
    offset = 0;
    prevLine = prevPage = nullptr;
    uintptr_t piece = pos / PIECESIZE;
    if (piece >= 1)
        w->fileInfo.loadPiece(piece - 1);
    w->fileInfo.loadPiece(piece);
    if (piece < w->fileInfo.pieceLoadedSize - 1)
        w->fileInfo.loadPiece(piece + 1);

    char *cpos = w->fileInfo.content + pos;
    char *p = w->fileInfo.findLastParaStart(cpos + 1);
    if (p) {
        assert(p <= cpos && cpos - p <= 4096);
        auto ls = findLineStop(p, cpos);
        if (!ls.empty()) {
            p = ls.back();
        }
    }
    else
        p = cpos;

    w->fileInfo.setCurrentPos(p - w->fileInfo.content);
    renewCache();
    update();
}

void WidgetFull::paintEvent(QPaintEvent *eve)
{
    /*QElapsedTimer et;
    et.start();*/
    if (needRedraw)
        renewCache();

    if (!w->fileInfo.file.isOpen()) {
        painter.begin(this);
        QRect re = eve->rect();
        painter.fillRect(re, w->paintInfo.background);
        int lrp = w->paintInfo.padding;
        int tbp = (height() - cache.height()) / 2;
        if (re.left() < lrp)
            re.setLeft(lrp);
        if (width() - re.right() <= lrp)
            re.setRight(width() - lrp - 1);
        if (re.top() < tbp)
            re.setTop(tbp);
        if (height() - re.bottom() <= tbp)
            re.setBottom(height() - tbp - 1);
        painter.drawPixmap(re.x(), re.y(), cache, re.x() - lrp, re.y() - tbp, re.width(), re.height());
        painter.end();
        return;
    }

    QRect re = eve->rect();
    if (re.left() < (int)w->paintInfo.padding)
        re.setLeft(w->paintInfo.padding);

    if (re.right() >= width() - (int)w->paintInfo.padding)
        re.setRight(width() - w->paintInfo.padding - 1);

    int lineHeight = w->paintInfo.linespace + w->paintInfo.fontm->height();
    int whichLine = (re.top() + offset) / lineHeight;
    int lineStartPos = re.top() + offset - whichLine * lineHeight - w->paintInfo.linespace;
    int startPos = re.top();
    if (lineStartPos < 0) {
        startPos -= lineStartPos;
        lineStartPos = 0;
    }
    int cacheLine = whichLine + cacheStartPos;
    if (cacheLine >= lineCount)
        cacheLine -= lineCount;
    int acl = actualLines();
    painter.begin(this);
    painter.fillRect(eve->rect(), w->paintInfo.background);
    if ((re.top() + offset) / lineHeight >= acl) {
        painter.end();
        return;
    }
    painter.drawPixmap(re.left(), startPos,
        cache,
        re.left() - w->paintInfo.padding,
        cacheLine * w->paintInfo.fontm->height() + lineStartPos,
        re.width(),
        w->paintInfo.fontm->height() - lineStartPos);
    int endLine = (re.bottom() + offset) / lineHeight;
    if (endLine >= acl)
        endLine = acl - 1;
    for (;;) {
        whichLine++;
        cacheLine++;
        if (cacheLine == lineCount)
            cacheLine = 0;

        if (whichLine < endLine) {
            painter.drawPixmap(re.left(), whichLine * lineHeight + w->paintInfo.linespace - offset,
                cache,
                re.left() - w->paintInfo.padding,
                cacheLine * w->paintInfo.fontm->height(),
                re.width(),
                w->paintInfo.fontm->height());
        }
        else if (whichLine == endLine) {
            int endPos = re.bottom() + offset - whichLine * lineHeight;
            if (endPos > lineHeight)
                endPos = lineHeight;

            if (endPos >= (int)w->paintInfo.linespace) {
                painter.drawPixmap(re.left(), whichLine * lineHeight + w->paintInfo.linespace - offset,
                    cache,
                    re.left() - w->paintInfo.padding,
                    cacheLine * w->paintInfo.fontm->height(),
                    re.width(),
                    endPos - w->paintInfo.linespace + 1);
            }
            break;
        }
        else
            break;
    }

    if (selectedStart && !w->textsInfo.empty()) {
        drawSelected();
    }

    painter.end();
    // qDebug(QString::number(et.elapsed()).toLocal8Bit());
}

void WidgetFull::renewCache()
{
    needRedraw = false;
    cacheStartPos = 0;
    w->textsInfo.clear();
    char *cpos = w->fileInfo.content + w->fileInfo.currentPos;

    prepareCachePainter();
    painter.fillRect(cache.rect(), w->paintInfo.background);

    if (w->fileInfo.file.isOpen() && w->fileInfo.content)
        textCache(0, lineCount - 1, cpos);
    else
        painter.drawText(cache.rect(), Qt::AlignCenter | Qt::TextWordWrap, WELCOMETEXT);

    painter.end();
}

void WidgetFull::addOffset(int value)
{
    if (w->fileInfo.currentPos >= (uintptr_t)w->fileInfo.file.size() || w->textsInfo.isEmpty()) {
        w->ui->actionAutoRoll->setChecked(false);
        return;
    }
    offset += value;
    int lineheight = w->paintInfo.fontm->height() + w->paintInfo.linespace;
    div_t dt = div(offset, lineheight);
    if (dt.quot > 0) {
        if (actualLines() <= dt.quot) {
            pageMoveDown();
            return;
        }
        else {
            for (int i = 0; i < dt.quot; i++) {
                lineMoveDown();
            }
        }

        offset = dt.rem;
    }

    if (!w->textsInfo.isEmpty() && actualLines() < lineCount) {
        char *np = getNextPos(w->textsInfo.back().contentPos);
        if (np < w->fileInfo.contentEnd) {
            int cs = cacheStartPos - 1;
            if (cs < 0)
                cs += lineCount;
            prepareCachePainter();
            painter.fillRect(0, cs * w->paintInfo.fontm->height(), cache.width(),
                w->paintInfo.fontm->height(), w->paintInfo.background);
            textCache(cs, 1, np);
            painter.end();
        }
    }
    update();
}

void WidgetFull::textCache(int start, int count, char *cpos)
{
    QTextDecoder decoder(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    int x = 0;
    start++;
    int y = start * w->paintInfo.fontm->height() - w->paintInfo.fontm->descent();
    char *lastPos = cpos;
    w->fileInfo.checkCurrentPiece();
    QChar lastChar;
    while (cpos < w->fileInfo.contentEnd) {
        QString s = decoder.toUnicode(cpos, 1);
        cpos++;
        if (!s.isEmpty()) {
            if (s == "\n") {
                if (w->paintInfo.smartReturn && !isEndChar(lastChar))
                    continue;
                if (x == 0)
                    continue;
                lastChar = s[0];
                w->textsInfo.push_back(TextInfo(s[0], x, y, lastPos));

                count--;
                if (count <= 0)
                    break;

                start++;
                if (start > lineCount)
                    start = 1;

                y = start * w->paintInfo.fontm->height() - w->paintInfo.fontm->descent();
                x = 0;
            }
            else {
                int fw;
                if (s != "\r") {
                    lastChar = s[0];
                    fw = w->paintInfo.fontm->HORIZONTALADVANCE(s[0]);
                    if (x != 0 && x + fw > cache.width()) {
                        count--;
                        if (count <= 0)
                            break;

                        start++;
                        if (start > lineCount)
                            start = 1;

                        y = start * w->paintInfo.fontm->height() - w->paintInfo.fontm->descent();
                        x = 0;
                    }
                    painter.drawText(x, y, s);
                }
                else
                    fw = 0;

                w->textsInfo.push_back(TextInfo(s[0], x, y, lastPos));
                x += fw;
            }
            lastPos = cpos;
        }
    }
}

QList<char *> WidgetFull::findLineStop(char *p, char *cp)
{
    QTextDecoder decoder(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    QList<char *> lineStop;
    int x = 0;
    char *lastCharStart = p;
    assert(cp - p <= 4096);
    while (p < cp) {
        QString s = decoder.toUnicode(p, 1);
        p++;
        if (!s.isEmpty() && s[0] != '\r' && s[0] != '\n') {
            int cw = w->paintInfo.fontm->HORIZONTALADVANCE(s[0]);
            if (x == 0) {
                x += cw;
            }
            else {
                x += cw;
                if (x > cache.width()) {
                    lineStop.push_back(lastCharStart);
                    x = cw;
                }
            }
            lastCharStart = p;
        }
    }
    return lineStop;
}

char *WidgetFull::fillLine(char *end)
{
    char *lastPos;
    end--;
    QByteArray ba;
    QTextDecoder decoder(w->fileInfo.codec, QTextCodec::IgnoreHeader);
    int x = 0;
    QString s;
    int i;
    if (w->fileInfo.codec->name() == "UTF-8")
        i = 1;
    else if (w->fileInfo.codec->name().startsWith("UTF-16"))
        i = 2;
    else
        i = 3;

    while (end > w->fileInfo.content) {
        if (i == 1) {
            if (*reinterpret_cast<unsigned char *>(end) < 0x80) {
                s = decoder.toUnicode(end, 1);
            }
            else if (*reinterpret_cast<unsigned char *>(end) >= 0xc0) {
                s = decoder.toUnicode(ba);
                ba.clear();
            }
            else
                ba.prepend(*end);
        }
        else if (i == 2) {
            end--;
            s = decoder.toUnicode(end, 2);
        }
        else {
            if (*reinterpret_cast<unsigned char *>(end) < 0x80) {
                s = decoder.toUnicode(end, 1);
            }
            else {
                if (ba.isEmpty())
                    ba.append(*end);
                else {
                    ba.prepend(*end);
                    s = decoder.toUnicode(ba);
                    ba.clear();
                }
            }
        }

        if (!s.isEmpty()) {
            if (s[0] == '\n')
                return end + 1;
            else if (s[0] != '\r') {
                int cw = w->paintInfo.fontm->HORIZONTALADVANCE(s);
                if (x != 0 && x + cw > cache.width()) {
                    return lastPos;
                }
                x += cw;
                lastPos = end;
            }
            s.clear();
        }
        end--;
    }
    return w->fileInfo.content;
}

int WidgetFull::actualLines()
{
    if (w->textsInfo.empty())
        return 0;

    int y1 = w->textsInfo.front().screenPos.y();
    int y2 = w->textsInfo.back().screenPos.y();
    y1 = (y1 + w->paintInfo.fontm->descent()) / w->paintInfo.fontm->height();
    y2 = (y2 + w->paintInfo.fontm->descent()) / w->paintInfo.fontm->height();
    if (y2 >= y1)
        return y2 - y1 + 1;
    else
        return lineCount - y1 + 1 + y2;
}

char *WidgetFull::getContentPosFromScreen(const QPoint &p)
{
    if (w->textsInfo.empty())
        return nullptr;

    int line = (p.y() + offset) / (w->paintInfo.fontm->height() + w->paintInfo.linespace);
    if (line < 0)
        line = 0;
    auto it = w->textsInfo.begin();
    int y = it->screenPos.y();
    while (line > 0) {
        it++;
        if (it == w->textsInfo.end())
            break;
        if (it->screenPos.y() != y) {
            line--;
            y = it->screenPos.y();
        }
    }

    if (it == w->textsInfo.end())
        return w->textsInfo.back().contentPos;

    int x = p.x() - w->paintInfo.padding;
    if (x <= 0)
        return it->contentPos;
    do {
        it++;
    } while (it != w->textsInfo.end() && it->screenPos.x() <= x && it->screenPos.y() == y);
    it--;
    return it->contentPos;
}

void WidgetFull::drawSelected()
{
    char *p;
    char *pe;
    if (selectedStart <= selectedEnd) {
        p = selectedStart;
        pe = selectedEnd;
    }
    else {
        p = selectedEnd;
        pe = selectedStart;
    }

    if (w->textsInfo.front().contentPos <= pe && w->textsInfo.back().contentPos >= p) {
        int startX, startLine, endX, endLine;
        auto it = w->textsInfo.begin();
        for (;;) {
            if (it->contentPos >= p) {
                startX = it->screenPos.x() + w->paintInfo.padding;
                startLine = (it->screenPos.y() + w->paintInfo.fontm->descent()) / w->paintInfo.fontm->height();
                startLine -= cacheStartPos;
                if (startLine <= 0)
                    startLine += lineCount;
                break;
            }
            it++;
        }

        if (pe >= w->textsInfo.back().contentPos) {
            endX = width() - 1;
            endLine = w->textsInfo.back().screenPos.y();
        }
        else {
            for (;;) {
                it++;
                if (it->contentPos > pe) {
                    QPoint point = it->screenPos;
                    it--;
                    if (it->screenPos.y() != point.y()) {
                        endX = width() - 1;
                        endLine = it->screenPos.y();
                    }
                    else {
                        endX = point.x() + w->paintInfo.padding - 1;
                        endLine = point.y();
                    }
                    break;
                }
            }
        }
        endLine = (endLine + w->paintInfo.fontm->descent()) / w->paintInfo.fontm->height() - cacheStartPos;
        if (endLine <= 0)
            endLine += lineCount;

        painter.setPen(w->paintInfo.selectedColor);
        int lsm = w->paintInfo.linespace / 2 - offset;
        int lineHeight = w->paintInfo.fontm->height() + w->paintInfo.linespace;
        if (startLine == endLine) {
            QRect rect(QPoint(startX, (startLine - 1) * lineHeight + lsm),
                QPoint(endX, endLine * lineHeight + lsm - 1));
            painter.drawRect(rect);
        }
        else if (startLine + 1 == endLine && startX > endX) {
            QRect r1(QPoint(startX, (startLine - 1) * lineHeight + lsm),
                QPoint(width() - 1, startLine * lineHeight + lsm - 1));
            painter.drawRect(r1);
            QRect r2(QPoint(0, startLine * lineHeight + lsm),
                QPoint(endX, endLine * lineHeight + lsm - 1));
            painter.drawRect(r2);
        }
        else {
            QPoint ps[8];
            ps[0].rx() = startX;
            ps[0].ry() = startLine * lineHeight + lsm;
            ps[1].rx() = startX;
            ps[1].ry() = ps[0].y() - lineHeight;
            ps[2].rx() = width() - 1;
            ps[2].ry() = ps[1].y();
            ps[3].rx() = ps[2].x();
            ps[3].ry() = (endLine - 1) * lineHeight + lsm;
            ps[4].rx() = endX;
            ps[4].ry() = ps[3].y();
            ps[5].rx() = endX;
            ps[5].ry() = ps[4].y() + lineHeight;
            ps[6].rx() = 0;
            ps[6].ry() = ps[5].y();
            ps[7].rx() = 0;
            ps[7].ry() = ps[0].y();
            painter.drawPolyline(ps, 8);
            painter.drawLine(ps[7], ps[0]);
        }
    }
}

void WidgetFull::onActionCopy()
{
    if (selectedStart) {
        char *p, *p2;
        if (selectedStart > selectedEnd) {
            p = selectedEnd;
            p2 = selectedStart;
        }
        else {
            p = selectedStart;
            p2 = selectedEnd;
        }

        QString s, text;
        QTextDecoder td(w->fileInfo.codec, QTextCodec::IgnoreHeader);
        while (p < w->fileInfo.contentEnd) {
            s = td.toUnicode(p, 1);
            if (!s.isEmpty()) {
                text += s;
                if (p >= p2)
                    break;
            }
            p++;
        }
        QApplication::clipboard()->setText(text);
    }
}

void WidgetFull::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || !w->fileInfo.file.isOpen())
        return;

    if (!dragging) {
        if (abs(event->pos().x() - pressPoint.x()) <= 3 && abs(event->pos().y() - pressPoint.y()) <= 3)
            return;
        else {
            dragging = true;
            selectedStart = getContentPosFromScreen(pressPoint);
        }
    }
    char *p = getContentPosFromScreen(event->pos());
    if (p != selectedEnd) {
        selectedEnd = p;
        update();
    }
}

void WidgetFull::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && w->fileInfo.file.isOpen())
        pressPoint = event->pos();
}

void WidgetFull::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !w->fileInfo.file.isOpen())
        return;

    if (!dragging) {
        if (selectedStart) {
            selectedStart = nullptr;
            selectedEnd = nullptr;
            update();
        }
    }
    else {
        dragging = false;
    }
}

void WidgetFull::contextMenuEvent(QContextMenuEvent *event)
{
    if (selectedStart) {
        popupMenu->popup(event->globalPos());
    }
    else {
        w->popupMenu.popup(event->globalPos());
    }
}

void WidgetFull::prepareCachePainter()
{
    painter.begin(&cache);
    painter.setPen(w->paintInfo.fontcolor);
    painter.setFont(w->paintInfo.font);
}
