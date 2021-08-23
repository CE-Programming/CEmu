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

#include "hexwidget.h"

#include "../../dockedwidget.h"
#include "memwidget.h"
#include "sliderpanner.h"
#include "../../util.h"

#include <QtCore/QHash>
#include <QtCore/QtGlobal>
#include <QtGui/QClipboard>
#include <QtGui/QFocusEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>

HexWidget::Pos::Pos(Area area, int off)
    : mArea{area},
      mOff{off}
{
}

bool HexWidget::Pos::isValid() const
{
    return mArea != Area::Addr;
}

bool HexWidget::Pos::isData() const
{
    return mArea == Area::Data;
}

bool HexWidget::Pos::isChar() const
{
    return mArea == Area::Char;
}

auto HexWidget::Pos::area() const -> Area
{
    return mArea;
}

bool HexWidget::Pos::low() const
{
    return mOff & 1;
}

bool HexWidget::Pos::high() const
{
    return !low();
}

int HexWidget::Pos::off() const
{
    return mOff;
}

int HexWidget::Pos::addr() const
{
    return mOff >> 1;
}

auto HexWidget::Pos::withArea(Area area) const -> Pos
{
    return {area, mOff};
}

auto HexWidget::Pos::withOff(int off) const -> Pos
{
    return {mArea, off};
}

auto HexWidget::Pos::withAddr(int addr) const -> Pos
{
    return {mArea, addr << 1};
}

auto HexWidget::Pos::withLow(bool low) const -> Pos
{
    return withOff((mOff & ~1) | low);
}

auto HexWidget::Pos::withHigh(bool high) const -> Pos
{
    return withLow(!high);
}

auto HexWidget::Pos::withMinOff(int off) const -> Pos
{
    return withOff(qMin(mOff, off));
}

auto HexWidget::Pos::withMaxOff(int off) const -> Pos
{
    return withOff(qMax(mOff, off));
}

auto HexWidget::Pos::off(int off) const -> Pos
{
    return withOff(mOff + off);
}

auto HexWidget::Pos::byteOff(int byteOff) const -> Pos
{
    return off(byteOff << 1);
}

int HexWidget::Pos::byteDiff(int off) const
{
    return qAbs(addr() - (off >> 1)) + 1;
}

HexWidget::HexWidget(MemWidget *parent, cemucore::prop prop, int len)
    : QAbstractScrollArea{parent},
      mProp{prop},
      mLastPos{(len << 1) - 1},
      mStride{32},
      mOff{},
      mCharset{Charset::TIAscii},
      mTopLine{},
      mCurPos{Area::Data, {}},
      mSelEnd{},
      mUndoPos{},
      mUndoStack{1024}
{
    setFont(Util::monospaceFont());

    auto *vBar = verticalScrollBar();
    vBar->setRange(-100, 100);
    connect(new SliderPanner{vBar}, &SliderPanner::panBy, [this](int amount)
    {
        mTopLine = qBound(0, mTopLine + (amount - qBound(-2, amount, 2)) *
                          qMax(1, qAbs(amount) - 50),
                          (mLastPos + mOff) / mStride - mVisibleLines + 2);
        viewport()->update();
    });

    resizeEvent();
}

MemWidget *HexWidget::parent() const
{
    return static_cast<MemWidget *>(QAbstractScrollArea::parent());
}

CoreWrapper &HexWidget::core() const
{
    return parent()->dockedWidget()->core();
}

int HexWidget::bytesPerLine() const
{
    return mStride >> 1;
}

void HexWidget::setBytesPerLine(int bytesPerLine)
{
    mStride = bytesPerLine << 1;
    resizeEvent();
}

int HexWidget::byteOff() const
{
    return mOff >> 1;
}

void HexWidget::setByteOff(int byteOff)
{
    mOff = byteOff << 1;
    resizeEvent();
}

auto HexWidget::charset() const -> Charset
{
    return mCharset;
}

void HexWidget::setCharset(Charset charset)
{
    mCharset = charset;
    if (charset == Charset::None && mCurPos.isChar())
    {
        mCurPos = mCurPos.withArea(Area::Data);
    }
    resizeEvent();
}

void HexWidget::gotoAddr(int addr)
{
    setCurPos(mCurPos.withAddr(addr), false);
}

void HexWidget::undo()
{
    if (mUndoStack.isEmpty() || mUndoPos < mUndoStack.firstIndex())
    {
        return;
    }
    auto &entry = mUndoStack.at(mUndoPos);
    core().set(mProp, entry.mPos.addr(), entry.mBefore);
    mUndoPos -= 1;
    setCurPos(entry.mPos, false);
    if (entry.mBefore.length() > 1)
    {
        setCurPos(entry.mPos.byteOff(entry.mBefore.length() - 1), true);
    }
}

void HexWidget::redo()
{
    if (mUndoStack.isEmpty() || mUndoPos >= mUndoStack.lastIndex())
    {
        return;
    }
    mUndoPos += 1;
    auto &entry = mUndoStack.at(mUndoPos);
    core().set(mProp, entry.mPos.addr(), entry.mAfter);
    setCurPos(entry.mPos, false);
    if (entry.mAfter.length() > 1)
    {
        setCurPos(entry.mPos.byteOff(entry.mAfter.length() - 1), true);
    }
}

void HexWidget::copy(bool selection)
{
    QClipboard *clip = QGuiApplication::clipboard();
    if (selection && !clip->supportsSelection())
    {
        return;
    }
    QString text;
    auto data = core().get(mProp, mCurPos.withMinOff(mSelEnd).addr(), mCurPos.byteDiff(mSelEnd));
    if (mCurPos.isChar())
    {
        text.reserve(data.length());
        for (char c : data)
        {
            text += charToUnicode(c);
        }
    }
    else
    {
        text = QString::fromLatin1(data.toHex().toUpper());
    }
    clip->setText(text, selection ? QClipboard::Selection : QClipboard::Clipboard);
}

void HexWidget::paste(bool selection)
{
    QClipboard *clip = QGuiApplication::clipboard();
    if (selection && !clip->supportsSelection())
    {
        return;
    }
    QString subtype = QStringLiteral("plain");
    QString text = clip->text(subtype, selection ? QClipboard::Selection : QClipboard::Clipboard);
    if (text.isEmpty())
    {
        return;
    }
    QByteArray data;
    if (mCurPos.isChar())
    {
        data.reserve(text.length());
        for (QChar c : text)
        {
            data += unicodeToChar(c);
        }
    }
    else
    {
        data = QByteArray::fromHex(text.toLatin1());
    }
    if (data.isEmpty())
    {
        return;
    }
    auto minPos = mCurPos.withMinOff(mSelEnd);
    overwriteRange(minPos, data);
    setCurPos(minPos.byteOff(data.length()), false);
}

QChar HexWidget::tiasciiToUnicode(char c, QChar placeholder)
{
    static const QChar sTIAsciiToUnicode[] =
    {
        0x0000, 0xF00E, 0x0000, 0x0000, 0x0000, 0xF014, 0x0000, 0xF016,
        0x222B, 0x00D7, 0x25AB, 0x207A, 0x2022, 0xF038, 0x00B3, 0xF02E,
        0x221A, 0xF005, 0x00B2, 0x2220, 0x00B0, 0xF001, 0xF002, 0x2264,
        0x2260, 0x2265, 0x00AF, 0xF000, 0x2192, 0xF01D, 0x2191, 0x2193,
        0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
        0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
        0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
        0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
        0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
        0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
        0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
        0x0058, 0x0059, 0x005A, 0x03B8, 0x005C, 0x005D, 0x005E, 0x005F,
        0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
        0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
        0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
        0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x0000,
        0x2080, 0x2081, 0x2082, 0x2083, 0x2084, 0x2085, 0x2086, 0x2087,
        0x2088, 0x2089, 0x00C1, 0x00C0, 0x00C2, 0x00C4, 0x00E1, 0x00E0,
        0x00E2, 0x00E4, 0x00C9, 0x00C8, 0x00CA, 0x00CB, 0x00E9, 0x00E8,
        0x00EA, 0x00EB, 0x00CD, 0x00CC, 0x00CE, 0x00CF, 0x00ED, 0x00EC,
        0x00EE, 0x00EF, 0x00D3, 0x00D2, 0x00D4, 0x00D6, 0x00F3, 0x00F2,
        0x00F4, 0x00F6, 0x00DA, 0x00D9, 0x00DB, 0x00DC, 0x00FA, 0x00F9,
        0x00FB, 0x00FC, 0x00C7, 0x00E7, 0x00D1, 0x00F1, 0x00B4, 0x0000,
        0x00A8, 0x00BF, 0x00A1, 0x03B1, 0x03B2, 0x03B3, 0x0394, 0x03B4,
        0x03B5, 0x005B, 0x03BB, 0x00B5, 0x03C0, 0x03C1, 0x03A3, 0x03C3,
        0x03C4, 0x03C6, 0x03A9, 0xF003, 0xF004, 0xF006, 0x2026, 0xF00B,
        0x0000, 0xF00A, 0x0000, 0x00B2, 0x00B0, 0x00B3, 0x000A, 0xF02F,
        0xF022, 0x03C7, 0xF021, 0x212F, 0x230A, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x2074, 0xF015, 0x00DF, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    };
    QChar r = sTIAsciiToUnicode[quint8(c)];
    return r.isNull() ? placeholder : r;
}

char HexWidget::unicodeToTIAscii(QChar c, char placeholder)
{
    static const QHash<QChar, char> sUnicodeToTIAscii
    {
        {0x000A, 0xD6}, {0x0020, 0x20}, {0x0021, 0x21}, {0x0022, 0x22},
        {0x0023, 0x23}, {0x0024, 0x24}, {0x0025, 0x25}, {0x0026, 0x26},
        {0x0027, 0x27}, {0x0028, 0x28}, {0x0029, 0x29}, {0x002A, 0x2A},
        {0x002B, 0x2B}, {0x002C, 0x2C}, {0x002D, 0x2D}, {0x002E, 0x2E},
        {0x002F, 0x2F}, {0x0030, 0x30}, {0x0031, 0x31}, {0x0032, 0x32},
        {0x0033, 0x33}, {0x0034, 0x34}, {0x0035, 0x35}, {0x0036, 0x36},
        {0x0037, 0x37}, {0x0038, 0x38}, {0x0039, 0x39}, {0x003A, 0x3A},
        {0x003B, 0x3B}, {0x003C, 0x3C}, {0x003D, 0x3D}, {0x003E, 0x3E},
        {0x003F, 0x3F}, {0x0040, 0x40}, {0x0041, 0x41}, {0x0042, 0x42},
        {0x0043, 0x43}, {0x0044, 0x44}, {0x0045, 0x45}, {0x0046, 0x46},
        {0x0047, 0x47}, {0x0048, 0x48}, {0x0049, 0x49}, {0x004A, 0x4A},
        {0x004B, 0x4B}, {0x004C, 0x4C}, {0x004D, 0x4D}, {0x004E, 0x4E},
        {0x004F, 0x4F}, {0x0050, 0x50}, {0x0051, 0x51}, {0x0052, 0x52},
        {0x0053, 0x53}, {0x0054, 0x54}, {0x0055, 0x55}, {0x0056, 0x56},
        {0x0057, 0x57}, {0x0058, 0x58}, {0x0059, 0x59}, {0x005A, 0x5A},
        {0x005B, 0xC1}, {0x005C, 0x5C}, {0x005D, 0x5D}, {0x005E, 0x5E},
        {0x005F, 0x5F}, {0x0060, 0x60}, {0x0061, 0x61}, {0x0062, 0x62},
        {0x0063, 0x63}, {0x0064, 0x64}, {0x0065, 0x65}, {0x0066, 0x66},
        {0x0067, 0x67}, {0x0068, 0x68}, {0x0069, 0x69}, {0x006A, 0x6A},
        {0x006B, 0x6B}, {0x006C, 0x6C}, {0x006D, 0x6D}, {0x006E, 0x6E},
        {0x006F, 0x6F}, {0x0070, 0x70}, {0x0071, 0x71}, {0x0072, 0x72},
        {0x0073, 0x73}, {0x0074, 0x74}, {0x0075, 0x75}, {0x0076, 0x76},
        {0x0077, 0x77}, {0x0078, 0x78}, {0x0079, 0x79}, {0x007A, 0x7A},
        {0x007B, 0x7B}, {0x007C, 0x7C}, {0x007D, 0x7D}, {0x007E, 0x7E},
        {0x00A1, 0xBA}, {0x00A8, 0xB8}, {0x00AF, 0x1A}, {0x00B0, 0xD4},
        {0x00B2, 0xD3}, {0x00B3, 0xD5}, {0x00B4, 0xB6}, {0x00B5, 0xC3},
        {0x00BF, 0xB9}, {0x00C0, 0x8B}, {0x00C1, 0x8A}, {0x00C2, 0x8C},
        {0x00C4, 0x8D}, {0x00C7, 0xB2}, {0x00C8, 0x93}, {0x00C9, 0x92},
        {0x00CA, 0x94}, {0x00CB, 0x95}, {0x00CC, 0x9B}, {0x00CD, 0x9A},
        {0x00CE, 0x9C}, {0x00CF, 0x9D}, {0x00D1, 0xB4}, {0x00D2, 0xA3},
        {0x00D3, 0xA2}, {0x00D4, 0xA4}, {0x00D6, 0xA5}, {0x00D7, 0x09},
        {0x00D9, 0xAB}, {0x00DA, 0xAA}, {0x00DB, 0xAC}, {0x00DC, 0xAD},
        {0x00DF, 0xF4}, {0x00E0, 0x8F}, {0x00E1, 0x8E}, {0x00E2, 0x90},
        {0x00E4, 0x91}, {0x00E7, 0xB3}, {0x00E8, 0x97}, {0x00E9, 0x96},
        {0x00EA, 0x98}, {0x00EB, 0x99}, {0x00EC, 0x9F}, {0x00ED, 0x9E},
        {0x00EE, 0xA0}, {0x00EF, 0xA1}, {0x00F1, 0xB5}, {0x00F2, 0xA7},
        {0x00F3, 0xA6}, {0x00F4, 0xA8}, {0x00F6, 0xA9}, {0x00F9, 0xAF},
        {0x00FA, 0xAE}, {0x00FB, 0xB0}, {0x00FC, 0xB1}, {0x0394, 0xBE},
        {0x03A3, 0xC6}, {0x03A9, 0xCA}, {0x03B1, 0xBB}, {0x03B2, 0xBC},
        {0x03B3, 0xBD}, {0x03B4, 0xBF}, {0x03B5, 0xC0}, {0x03B8, 0x5B},
        {0x03BB, 0xC2}, {0x03C0, 0xC4}, {0x03C1, 0xC5}, {0x03C3, 0xC7},
        {0x03C4, 0xC8}, {0x03C6, 0xC9}, {0x03C7, 0xD9}, {0x2022, 0x0C},
        {0x2026, 0xCE}, {0x2074, 0xF2}, {0x207A, 0x0B}, {0x2080, 0x80},
        {0x2081, 0x81}, {0x2082, 0x82}, {0x2083, 0x83}, {0x2084, 0x84},
        {0x2085, 0x85}, {0x2086, 0x86}, {0x2087, 0x87}, {0x2088, 0x88},
        {0x2089, 0x89}, {0x212F, 0xDB}, {0x2191, 0x1E}, {0x2192, 0x1C},
        {0x2193, 0x1F}, {0x221A, 0x10}, {0x2220, 0x13}, {0x222B, 0x08},
        {0x2260, 0x18}, {0x2264, 0x17}, {0x2265, 0x19}, {0x230A, 0xDC},
        {0x25AB, 0x0A}, {0xF000, 0x1B}, {0xF001, 0x15}, {0xF002, 0x16},
        {0xF003, 0xCB}, {0xF004, 0xCC}, {0xF005, 0x11}, {0xF006, 0xCD},
        {0xF00A, 0xD1}, {0xF00B, 0xCF}, {0xF00E, 0x01}, {0xF014, 0x05},
        {0xF015, 0xF3}, {0xF016, 0x07}, {0xF01D, 0x1D}, {0xF021, 0xDA},
        {0xF022, 0xD8}, {0xF02E, 0x0F}, {0xF02F, 0xD7}, {0xF038, 0x0D},
    };
    return sUnicodeToTIAscii.value(c, placeholder);
}

QChar HexWidget::charToUnicode(char c) const
{
    if (mCharset == Charset::TIAscii)
    {
        return tiasciiToUnicode(c);
    }
    QChar r = QChar::fromLatin1(c);
    return r.isPrint() ? r : QLatin1Char{'.'};
}

char HexWidget::unicodeToChar(QChar c) const
{
    if (mCharset == Charset::TIAscii)
    {
        return unicodeToTIAscii(c);
    }
    return c.toLatin1();
}

auto HexWidget::atLineStart(Pos pos) const -> Pos
{
    return pos.off(-((pos.off() + mOff) % mStride));
}

auto HexWidget::atLineEnd(Pos pos) const -> Pos
{
    return atLineStart(pos).off(mStride - 1);
}

QSize HexWidget::cellSize() const
{
    const auto &metrics = fontMetrics();
    return {metrics.maxWidth(), metrics.lineSpacing()};
}

QRect HexWidget::posToCell(Pos pos) const
{
    auto size = cellSize();
    int visOff = pos.off() + mOff, x = visOff % mStride;
    switch (pos.area())
    {
        case Area::Addr:
            x = 1;
            break;
        case Area::Data:
            x = (x * 3 >> 1) + 8;
            break;
        case Area::Char:
            x = ((mStride * 3 + x) >> 1) + 8;
            break;
    }
    return {{x * size.width() - (size.width() >> 1), visOff / mStride * size.height()}, size};
}

auto HexWidget::absToPos(QPoint abs) const -> Pos
{
    auto size = cellSize();
    int x = (abs.x() << 1) / size.width() + 1;
    int hexWidth = mStride * 3 >> 1;
    int visOff = abs.y() / size.height() * mStride - mOff;
    if (mCharset != Charset::None && x >> 1 >= hexWidth + 8 && x >> 1 < (mStride << 1) + 8)
    {
        return {Area::Char, visOff + x - (hexWidth << 1) - 16};
    }
    if (x >= 15 && x < (hexWidth << 1) + 15)
    {
        return {Area::Data, visOff + x / 3 - 5};
    }
    return {Area::Addr, visOff};
}

auto HexWidget::locToPos(QPoint loc) const -> Pos
{
    return absToPos(loc + QPoint{horizontalScrollBar()->value(), mTopLine * cellSize().height()});
}

void HexWidget::setCurPos(Pos pos, bool select)
{
    mCurPos = mCurPos.withArea(pos.area());
    mSelEnd = qBound(0, pos.off(), mLastPos);
    if (!select)
    {
        mCurPos = mCurPos.withOff(mSelEnd);
    }
    else if (mCurPos.off() != mSelEnd)
    {
        copy(true);
    }
    int line = (mSelEnd + mOff) / mStride;
    mTopLine = qBound(line - mVisibleLines + 2, mTopLine, line);
    auto cell = posToCell({mCurPos.area(), mSelEnd});
    auto *hBar = horizontalScrollBar();
    auto *view = viewport();
    hBar->setValue(qBound(cell.right() + 1 - view->width(), hBar->value(), cell.left()));
    view->update();
}

void HexWidget::overwriteChar(Pos pos, char c)
{
    overwriteRange(pos, {1, c});
}

void HexWidget::overwriteRange(Pos pos, QByteArray data)
{
    if (data.isEmpty())
    {
        return;
    }
    while (!mUndoStack.isEmpty() && mUndoPos != mUndoStack.lastIndex())
    {
        mUndoStack.removeLast();
    }
    data.truncate(pos.byteDiff(mLastPos));
    auto lock = core().lock();
    mUndoStack.append({pos, core().get(mProp, pos.addr(), data.length()), data});
    mUndoStack.normalizeIndexes();
    mUndoPos = mUndoStack.lastIndex();
    core().set(mProp, pos.addr(), data);
}

void HexWidget::overwriteNibble(Pos pos, int digit)
{
    while (!mUndoStack.isEmpty() && mUndoPos != mUndoStack.lastIndex())
    {
        mUndoStack.removeLast();
    }
    auto lock = core().lock();
    char before = core().get(mProp, pos.addr()), after;
    if (pos.low())
    {
        after = (before & 0xF0) | (digit << 0 & 0x0F);
    }
    else
    {
        after = (before & 0x0F) | (digit << 4 & 0xF0);
    }
    mUndoStack.append({pos, {1, before}, {1, after}});
    mUndoStack.normalizeIndexes();
    mUndoPos = mUndoStack.lastIndex();
    core().set(mProp, pos.addr(), after);
}

void HexWidget::resizeEvent(QResizeEvent *)
{
    mStride = qMax(1, mStride);
    mOff = qBound(0, mOff, mStride - 1);
    auto cell = posToCell({mCharset == Charset::None ? Area::Data : Area::Char, mStride - 1});
    const auto *view = viewport();
    int viewWidth = view->width(), viewHeight = view->height();
    auto *hBar = horizontalScrollBar();
    hBar->setRange(0, cell.right() + (cell.width() >> 1) - viewWidth);
    hBar->setPageStep(viewWidth);
    mVisibleLines = (viewHeight + cell.height() - 1) / cell.height();
    mTopLine = qBound(0, mTopLine, (mLastPos + mOff) / mStride - mVisibleLines + 2);
    viewport()->update();
}

void HexWidget::focusInEvent(QFocusEvent *event)
{
    event->accept();
}

void HexWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::MoveToPreviousChar))
    {
        setCurPos(mCurPos.off(-(mCurPos.isChar() + 1)), false);
    }
    else if (event->matches(QKeySequence::MoveToNextChar))
    {
        setCurPos(mCurPos.off(+(mCurPos.isChar() + 1)), false);
    }
    if (event->matches(QKeySequence::MoveToPreviousWord))
    {
        setCurPos(mCurPos.byteOff(-1).withLow(), false);
    }
    else if (event->matches(QKeySequence::MoveToNextWord))
    {
        setCurPos(mCurPos.byteOff(+1).withHigh(mCurPos.isChar()), false);
    }
    else if (event->matches(QKeySequence::MoveToPreviousLine))
    {
        setCurPos(mCurPos.off(-mStride), false);
    }
    else if (event->matches(QKeySequence::MoveToNextLine))
    {
        setCurPos(mCurPos.off(+mStride), false);
    }
    else if (event->matches(QKeySequence::MoveToStartOfLine))
    {
        setCurPos(atLineStart(mCurPos), false);
    }
    else if (event->matches(QKeySequence::MoveToEndOfLine))
    {
        setCurPos(atLineEnd(mCurPos).withHigh(mCurPos.isChar()), false);
    }
    else if (event->matches(QKeySequence::MoveToPreviousPage))
    {
        setCurPos(mCurPos.off(-mVisibleLines * mStride), false);
    }
    else if (event->matches(QKeySequence::MoveToNextPage))
    {
        setCurPos(mCurPos.off(+mVisibleLines * mStride), false);
    }
    else if (event->matches(QKeySequence::MoveToStartOfDocument))
    {
        setCurPos(mCurPos.withOff(0), false);
    }
    else if (event->matches(QKeySequence::MoveToEndOfDocument))
    {
        setCurPos(mCurPos.withOff(mLastPos), false);
    }
    if (event->matches(QKeySequence::SelectPreviousChar))
    {
        setCurPos(mCurPos.withOff(mSelEnd - (mCurPos.isChar() ||
                                             ((mCurPos.withLow().off() >= mSelEnd) ^
                                              mSelEnd) & 1) - 1), true);
    }
    else if (event->matches(QKeySequence::SelectNextChar))
    {
        setCurPos(mCurPos.withOff(mSelEnd + (mCurPos.isChar() ||
                                             ((mCurPos.off() > (mSelEnd | 1)) ^
                                              mSelEnd) & 1) + 1), true);
    }
    if (event->matches(QKeySequence::SelectPreviousWord))
    {
        setCurPos(mCurPos.withOff(mSelEnd - 2).withHigh(), true);
    }
    else if (event->matches(QKeySequence::SelectNextWord))
    {
        setCurPos(mCurPos.withOff(mSelEnd + 2).withLow(mCurPos.isChar()), true);
    }
    else if (event->matches(QKeySequence::SelectPreviousLine))
    {
        setCurPos(mCurPos.withOff(mSelEnd - mStride), true);
    }
    else if (event->matches(QKeySequence::SelectNextLine))
    {
        setCurPos(mCurPos.withOff(mSelEnd + mStride), true);
    }
    else if (event->matches(QKeySequence::SelectStartOfLine))
    {
        setCurPos(atLineStart(mCurPos.withOff(mSelEnd)), true);
    }
    else if (event->matches(QKeySequence::SelectEndOfLine))
    {
        setCurPos(atLineEnd(mCurPos.withOff(mSelEnd)).withHigh(mCurPos.isChar()), true);
    }
    else if (event->matches(QKeySequence::SelectPreviousPage))
    {
        setCurPos(mCurPos.withOff(mSelEnd - mVisibleLines * mStride), true);
    }
    else if (event->matches(QKeySequence::SelectNextPage))
    {
        setCurPos(mCurPos.withOff(mSelEnd + mVisibleLines * mStride), true);
    }
    else if (event->matches(QKeySequence::SelectStartOfDocument))
    {
        setCurPos(mCurPos.withOff(0), true);
    }
    else if (event->matches(QKeySequence::SelectEndOfDocument))
    {
        setCurPos(mCurPos.withOff(mLastPos), true);
    }
    else if (event->matches(QKeySequence::SelectAll))
    {
        setCurPos(mCurPos.withOff(0), false);
        setCurPos(mCurPos.withOff(mLastPos), true);
    }
    else if (event->matches(QKeySequence::Deselect))
    {
        setCurPos(mCurPos, false);
    }
    else if (event->matches(QKeySequence::Undo))
    {
        undo();
    }
    else if (event->matches(QKeySequence::Redo))
    {
        redo();
    }
    else if (event->matches(QKeySequence::Copy))
    {
        copy();
    }
    else if (event->matches(QKeySequence::Paste))
    {
        paste();
    }
#if 0
    else if (event->matches(QKeySequence::Backspace))
    {
        if (mCurPos.off() != mSelEnd || mCurPos.isChar())
        {
            overwriteRange(mCurPos.minOff(mSelEnd), {mCurPos.byteDiff(mSelEnd), 0});
            if (mCurPos.off() != mSelEnd)
            {
                setCurPos(mCurPos.minOff(mSelEnd).withHigh(), false);
            }
            else
            {
                setCurPos(mCurPos.off(-2), false);
            }
        }
        else
        {
            overwriteNibble(mCurPos, 0);
            setCurPos(mCurPos.off(-1), false);
        }
    }
#endif
    else if (!(event->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier)) &&
             !event->text().isEmpty())
    {
        if (!mCurPos.isChar())
        {
            bool ok;
            int digit = event->text().toInt(&ok, 16);
            if (ok)
            {
                overwriteNibble(mCurPos, digit);
                setCurPos(mCurPos.off(+1), false);
            }
        }
        else if (char c = unicodeToChar(event->text().front()))
        {
            overwriteChar(mCurPos, c);
            setCurPos(mCurPos.byteOff(+1), false);
        }
    }
}

void HexWidget::mousePressEvent(QMouseEvent *event)
{
    bool select = event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier;
    auto pos = locToPos(event->pos());
    if (pos.isValid() && (!select || pos.area() == mCurPos.area()))
    {
        setCurPos(pos, select);
        event->accept();
    }
}

void HexWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }
    auto pos = locToPos(event->pos());
    if (pos.area() == mCurPos.area())
    {
        setCurPos(pos, true);
        event->accept();
    }
}

void HexWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::MiddleButton)
    {
        return;
    }
    auto pos = locToPos(event->pos());
    if (pos.area() == mCurPos.area())
    {
        setCurPos(pos, false);
        paste(true);
        event->accept();
    }
}

void HexWidget::paintEvent(QPaintEvent *event)
{
    const QPalette &palette = viewport()->palette();
    QRect region = event->rect();
    QPainter painter{viewport()};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(region, palette.color(QPalette::Base));
    painter.translate(-horizontalScrollBar()->value(), 0);

    auto minPos = mCurPos.withMinOff(mSelEnd).withHigh(),
         maxPos = mCurPos.withMaxOff(mSelEnd).withLow();

    const auto drawSep = [&](Area area)
    {
        auto cell = posToCell({area, mStride - mOff});
        auto x = cell.x() - (cell.width() >> 1);
        painter.drawLine(x, region.top(), x, region.bottom() + 1);
    };

    const auto textColor = [&](Pos pos)
    {
        painter.setPen(palette.color(pos.area() == mCurPos.area() && mCurPos.off() != mSelEnd &&
                                     minPos.off() <= pos.off() && pos.off() <= maxPos.off()
                                     ? QPalette::HighlightedText : QPalette::WindowText));
    };

    const auto drawText = [&](Pos pos, const QString &text)
    {
        textColor(pos);
        auto cell = posToCell(pos);
        painter.drawText(cell.x(), cell.y() + fontMetrics().ascent(), text);
    };

    painter.setPen(Qt::gray);
    drawSep(Area::Data);
    if (mCharset != Charset::None)
    {
        drawSep(Area::Char);
    }

    painter.translate(0, -posToCell({Area::Addr, mTopLine * mStride}).top());
    if (mCurPos.off() != mSelEnd)
    {
        auto highlight = palette.color(QPalette::Highlight);
        auto min = posToCell(minPos), max = posToCell(maxPos);
        if (min.top() == max.top())
        {
            painter.fillRect(min | max, highlight);
        }
        else
        {
            auto rightPos = atLineEnd(minPos), leftPos = atLineStart(maxPos);
            painter.fillRect(min | posToCell(rightPos), highlight);
            if (min.bottom() + 1 < max.top())
            {
                painter.fillRect(posToCell(rightPos.off(+1)) | posToCell(leftPos.off(-1)), highlight);
            }
            painter.fillRect(posToCell(leftPos) | max, highlight);
        }
    }

    int pos = mTopLine * mStride - mOff, startPos = qMax(0, pos);
    auto data = core().get(mProp, startPos >> 1,
                           qMin(mVisibleLines * mStride,
                                mLastPos + 1 - startPos) >> 1);
    auto hex = QString::fromLatin1(data.toHex().toUpper());
    for (int line = 0, linePos = 0, startIndex = 0, startCol = mTopLine ? 0 : mOff;
         line < mVisibleLines && startIndex < hex.length();
         line += 1, linePos += mStride, startIndex += mStride - startCol, startCol = 0)
    {
        drawText({Area::Addr, pos + linePos}, QStringLiteral("%1")
                 .arg(QString::number(qMax(0, pos + linePos) >> 1, 16).toUpper(), 6, QLatin1Char{'0'}));
        for (int colPos = startCol, index = startIndex;
             colPos < mStride && index < hex.length(); colPos += 1, index += 1)
        {
            drawText({Area::Data, pos + linePos + colPos}, hex.mid(index, 1));
        }
        if (mCharset != Charset::None)
        {
            for (int colPos = startCol, index = startIndex >> 1;
                 colPos < mStride && index < data.length(); colPos += 2, index += 1)
            {
                drawText({Area::Char, pos + linePos + colPos},
                         {1, charToUnicode(data.at(index))});
            }
        }
    }

    {
        textColor(mCurPos);
        auto cell = posToCell(mCurPos);
        auto y = cell.top() + fontMetrics().ascent() + fontMetrics().underlinePos();
        painter.drawLine(cell.left(), y, cell.right(), y);
    }
}
