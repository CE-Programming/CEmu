/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEXWIDGET_H
#define HEXWIDGET_H

#include "../../corewrapper.h"
class CoreWrapper;
class MemWidget;

#include <QtCore/QByteArray>
#include <QtCore/QChar>
#include <QtCore/QContiguousCache>
#include <QtCore/QLatin1Char>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QTimer>
#include <QtWidgets/QAbstractScrollArea>
QT_BEGIN_NAMESPACE
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QTimerEvent;
QT_END_NAMESPACE

class HexWidget : public QAbstractScrollArea
{
    Q_OBJECT

    enum class Area
    {
        Addr,
        Data,
        Char,
    };

    class Pos
    {
    public:
        Pos(Area area, int off);

        bool isValid() const;
        bool isData() const;
        bool isChar() const;
        Area area() const;

        bool low() const;
        bool high() const;
        int off() const;
        int addr() const;

        Pos withArea(Area area) const;
        Pos withOff(int off) const;
        Pos withLow(bool low = true) const;
        Pos withHigh(bool high = true) const;
        Pos withMinOff(int off) const;
        Pos withMaxOff(int off) const;
        Pos off(int off) const;
        Pos byteOff(int byteOff) const;
        int byteDiff(int off) const;
        Pos atLineStart(int stride) const;
        Pos atLineEnd(int stride) const;

    private:
        Area mArea;
        int mOff;
    };

    struct UndoEntry
    {
        Pos mPos;
        QByteArray mBefore;
        QByteArray mAfter;
    };

public:
    enum class Charset : quint8
    {
        None,
        Ascii,
        TIAscii,
    };

    HexWidget(MemWidget *parent, cemucore::prop prop, int len);
    MemWidget *parent() const;
    CoreWrapper &core() const;

    int bytesPerLine() const;
    void setBytesPerLine(int bytesPerLine);
    Charset charset() const;
    void setCharset(Charset charset);

public slots:
    void undo();
    void redo();
    void copy(bool selection = false);
    void paste(bool selection = false);

private:
    static QChar tiasciiToUnicode(char c, QChar placeholder = QLatin1Char{'.'});
    static char unicodeToTIAscii(QChar c, char placeholder = '\0');

    QChar charToUnicode(char c) const;
    char unicodeToChar(QChar c) const;

    QSize cellSize() const;
    QRect posToCell(Pos pos) const;
    Pos absToPos(QPoint abs) const;
    Pos locToPos(QPoint loc) const;

    void setCurPos(Pos pos, bool select);
    void overwriteChar(Pos pos, char c);
    void overwriteRange(Pos pos, QByteArray data);
    void overwriteNibble(Pos pos, int digit);

    void resizeEvent(QResizeEvent *event = nullptr) override;
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    cemucore::prop mProp;
    int mLastPos, mStride;
    Charset mCharset;

    int mTopLine, mVisibleLines;
    Pos mCurPos;
    int mSelEnd;

    int mUndoPos;
    QContiguousCache<UndoEntry> mUndoStack;
};

#endif
