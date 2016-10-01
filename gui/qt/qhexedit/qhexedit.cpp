#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>

#include "qhexedit.h"

/* Constructor */
QHexEdit::QHexEdit(QWidget *par) : QAbstractScrollArea(par) {
    _chunks = new Chunks();
    _undoStack = new UndoStack(_chunks, this);

#ifdef Q_OS_WIN32
    setFont(QFont("Courier", 10));
#else
    setFont(QFont("Monospace", 10));
#endif

    setHighlightingColor(QColor(Qt::blue).lighter(160));
    setSelectionColor(QColor(Qt::yellow).lighter(160));

    connect(&_cursorTimer, &QTimer::timeout, this, &QHexEdit::updateCursor);
    connect(_undoStack, &UndoStack::indexChanged, this, &QHexEdit::dataChangedPrivate);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &QHexEdit::adjust);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &QHexEdit::adjust);

    _cursorTimer.setInterval(500);
    _cursorTimer.start();
    _cursorPosition = 0;
    _addressWidth = 6;

    setAsciiArea(true);
    setAddressArea(true);
    setHighlighting(true);

    init();
}

QHexEdit::~QHexEdit() {
    delete _undoStack;
    delete _chunks;
}

/* Properties */
void QHexEdit::setAddressArea(bool addressArea_) {
    _addressArea = addressArea_;
    adjust();
    setCursorPosition(_cursorPosition);
    viewport()->update();
}

bool QHexEdit::addressArea() {
    return _addressArea;
}

void QHexEdit::setAddressOffset(qint64 addressOffset_) {
    _addressOffset = addressOffset_;
    adjust();
    setCursorPosition(_cursorPosition);
    viewport()->update();
}

qint64 QHexEdit::addressOffset() {
    return _addressOffset;
}

int QHexEdit::addressWidth() {
    qint64 sizeT = _chunks->size();
    int n = 1;
    if (sizeT > Q_INT64_C(0x100000000)){ n += 8; sizeT /= Q_INT64_C(0x100000000);}
    if (sizeT > 0x10000){ n += 4; sizeT /= 0x10000;}
    if (sizeT > 0x100){ n += 2; sizeT /= 0x100;}
    if (sizeT > 0x10){ n += 1; }

    if (n > _addressWidth) {
        return n;
    } else {
        return _addressWidth;
    }
}

void QHexEdit::setAsciiArea(bool asciiArea_) {
    _asciiArea = asciiArea_;
    viewport()->update();
}

bool QHexEdit::asciiArea() {
    return _asciiArea;
}

void QHexEdit::setCursorPosition(qint64 position) {
    int pxOfsX = horizontalScrollBar()->value();

    // Delete old cursor
    _blink = false;
    viewport()->update(_cursorRect);

    // Check if cursor is in range
    if (position > (_chunks->size() * 2 - 1)) {
        position = _chunks->size() * 2 - 1;
    }
    if (position < 0) {
        position = 0;
    }

    // Calculate new position of cursor
    _cursorPosition = position;
    _bPosCurrent = position / 2;
    _pxCursorY = ((position/2 - _bPosFirst) / bytesPerLine + 1) * _pxCharHeight;
    int xT = (position % (2 * bytesPerLine));
    _pxCursorX = (((xT / 2) * 3) + (xT % 2)) * _pxCharWidth + _pxPosHexX - pxOfsX;

    _cursorRect = QRect(_pxCursorX, _pxCursorY + _pxCursorWidth, _pxCharWidth, _pxCursorWidth);

    // Immiedately draw new cursor
    _blink = true;
    viewport()->update(_cursorRect);
    emit currentAddressChanged(_bPosCurrent);
}

qint64 QHexEdit::cursorPosition(QPoint posa) {
    int posx = posa.x() + horizontalScrollBar()->value();

    // Calculate cursor position depending on a graphical position
    qint64 result = -1;
    if ((posx >= _pxPosHexX) && (posx < (_pxPosHexX + ((bytesPerLine*2)+(bytesPerLine/2)+3) * _pxCharWidth))) {
        int xT = (posx - _pxPosHexX - _pxCharWidth / 2) / _pxCharWidth;
        xT = (xT / 3) * 2 + xT % 3;
        int yT = ((posa.y() - 3) / _pxCharHeight) * 2 * bytesPerLine;
        result = _bPosFirst * 2 + xT + yT;
    }
    return result;
}

qint64 QHexEdit::cursorPosition() {
    return _cursorPosition;
}

void QHexEdit::setData(const QByteArray &ba) {
    _data = ba;
    _bData.setData(_data);
    setData(_bData);
}

QByteArray QHexEdit::data() {
    return _chunks->data(0, -1);
}

void QHexEdit::setHighlighting(bool highlighting_) {
    _highlighting = highlighting_;
    viewport()->update();
}

bool QHexEdit::highlighting() {
    return _highlighting;
}

void QHexEdit::setHighlightingColor(const QColor &color) {
    _brushHighlighted = QBrush(color);
    _penHighlighted = QPen(viewport()->palette().color(QPalette::WindowText));
    viewport()->update();
}

QColor QHexEdit::highlightingColor() {
    return _brushHighlighted.color();
}

void QHexEdit::setSelectionColor(const QColor &color) {
    _brushSelection = QBrush(color);
    _penSelection = QPen(Qt::gray);
    viewport()->update();
}

QColor QHexEdit::selectionColor() {
    return _brushSelection.color();
}

/* Access to data of qhexedit */
bool QHexEdit::setData(QIODevice &iODevice) {
    bool ok = _chunks->setIODevice(iODevice);
    init();
    dataChangedPrivate();
    return ok;
}

QByteArray QHexEdit::dataAt(qint64 posa, qint64 count) {
    return _chunks->data(posa, count);
}

bool QHexEdit::write(QIODevice &iODevice, qint64 posa, qint64 count) {
    return _chunks->write(iODevice, posa, count);
}

/* Char handling */
void QHexEdit::replace(qint64 index, char ch) {
    _undoStack->overwrite(index, ch);
    refresh();
}

/* ByteArray handling */
void QHexEdit::replace(qint64 posa, qint64 len, const QByteArray &ba) {
    _undoStack->overwrite(posa, len, ba);
    refresh();
}

/* Utility functions */
void QHexEdit::ensureVisible() {

    if (_cursorPosition < (_bPosFirst * 2)) {
        verticalScrollBar()->setValue((int)(_cursorPosition / 2 / bytesPerLine));
    }
    if (_cursorPosition > ((_bPosFirst + (_rowsShown - 1) * bytesPerLine) * 2)) {
        verticalScrollBar()->setValue((int)(_cursorPosition / 2 / bytesPerLine) - _rowsShown + 1);
    }
    if (_pxCursorX < horizontalScrollBar()->value()) {
        horizontalScrollBar()->setValue(0);
    }

    viewport()->update();
}

void QHexEdit::setBytesPerLine(int bytes) {
    bytesPerLine = bytes;
    adjust();
    ensureVisible();
}

qint64 QHexEdit::indexOf(const QByteArray &ba, qint64 from) {
    qint64 posa = _chunks->indexOf(ba, from/2);
    if (posa > -1) {
        qint64 curPos = posa*2;
        setCursorPosition(curPos + ba.length()*2);
        resetSelection(curPos);
        setSelection(curPos + ba.length()*2);
        ensureVisible();
    }
    return posa;
}

bool QHexEdit::isModified() {
    return _modified;
}

qint64 QHexEdit::lastIndexOf(const QByteArray &ba, qint64 from) {
    qint64 posa = _chunks->lastIndexOf(ba, from/2);

    if (posa > -1) {
        qint64 curPos = posa*2;
        setCursorPosition(curPos - 1);
        resetSelection(curPos);
        setSelection(curPos + ba.length()*2);
        ensureVisible();
    }

    return posa;
}

void QHexEdit::redo() {
    _undoStack->redo();
    setCursorPosition(_chunks->pos()*2);
    refresh();
}

QString QHexEdit::selectionToReadableString() {
    QByteArray ba = _chunks->data(getSelectionBegin(), getSelectionEnd() - getSelectionBegin());
    return toReadable(ba);
}

void QHexEdit::setFont(const QFont &font_) {
    QWidget::setFont(font_);
    _pxCharWidth = fontMetrics().width(QLatin1Char('D'));
    _pxCharHeight = fontMetrics().height();
    _pxGapAdr = _pxCharWidth / 2;
    _pxGapAdrHex = _pxCharWidth;
    _pxGapHexAscii = _pxCharWidth;
    _pxCursorWidth = _pxCharHeight / 7;
    _pxSelectionSub = _pxCharHeight / 5;
    viewport()->update();
}

QString QHexEdit::toReadableString() {
    QByteArray ba = _chunks->data();
    return toReadable(ba);
}

void QHexEdit::undo() {
    _undoStack->undo();
    setCursorPosition(_chunks->pos()*2);
    refresh();
}

/* Handle events */
void QHexEdit::keyPressEvent(QKeyEvent *e) {

    /* Cursor movements */
    if (e->matches(QKeySequence::MoveToNextChar)) {
        setCursorPosition(_cursorPosition + 1);
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToPreviousChar)) {
        setCursorPosition(_cursorPosition - 1);
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToEndOfLine)) {
        setCursorPosition(_cursorPosition | (2 * bytesPerLine -1));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToStartOfLine)) {
        setCursorPosition(_cursorPosition - (_cursorPosition % (2 * bytesPerLine)));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToPreviousLine)) {
        setCursorPosition(_cursorPosition - (2 * bytesPerLine));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToNextLine)) {
        setCursorPosition(_cursorPosition + (2 * bytesPerLine));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToNextPage)) {
        setCursorPosition(_cursorPosition + (((_rowsShown - 1) * 2 * bytesPerLine)));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToPreviousPage)) {
        setCursorPosition(_cursorPosition - (((_rowsShown - 1) * 2 * bytesPerLine)));
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToEndOfDocument)) {
        setCursorPosition(_chunks->size() * 2);
        resetSelection(_cursorPosition);
    }

    if (e->matches(QKeySequence::MoveToStartOfDocument)){
        setCursorPosition(0);
        resetSelection(_cursorPosition);
    }

    /* Select commands */
    if (e->matches(QKeySequence::SelectNextChar)) {
        qint64 posa = _cursorPosition + 1;
        setCursorPosition(posa);
        setSelection(posa);
    }

    if (e->matches(QKeySequence::SelectPreviousChar)) {
        qint64 posa = _cursorPosition - 1;
        setSelection(posa);
        setCursorPosition(posa);
    }

    if (e->matches(QKeySequence::SelectEndOfLine)) {
        qint64 posa = _cursorPosition - (_cursorPosition % (2 * bytesPerLine)) + (2 * bytesPerLine);
        setCursorPosition(posa);
        setSelection(posa);
    }

    if (e->matches(QKeySequence::SelectStartOfLine)) {
        qint64 posa = _cursorPosition - (_cursorPosition % (2 * bytesPerLine));
        setCursorPosition(posa);
        setSelection(posa);
    }

    if (e->matches(QKeySequence::SelectPreviousLine)) {
        qint64 posa = _cursorPosition - (2 * bytesPerLine);
        setCursorPosition(posa);
        setSelection(posa);
    }

    if (e->matches(QKeySequence::SelectNextLine)) {
        qint64 posa = _cursorPosition + (2 * bytesPerLine);
        setCursorPosition(posa);
        setSelection(posa);
    }

    if (e->matches(QKeySequence::SelectNextPage)) {
        qint64 posa = _cursorPosition + (((viewport()->height() / _pxCharHeight) - 1) * 2 * bytesPerLine);
        setCursorPosition(posa);
        setSelection(posa);
    }
    if (e->matches(QKeySequence::SelectPreviousPage)) {
        qint64 posa = _cursorPosition - (((viewport()->height() / _pxCharHeight) - 1) * 2 * bytesPerLine);
        setCursorPosition(posa);
        setSelection(posa);
    }

    /* Edit Commands */
    if (!(e->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier))) {
        /* Hex input */
        int key = e->key();
        if ((key>='0' && key<='9') || (key>='A' && key <= 'F')) {
            if (getSelectionBegin() != getSelectionEnd()) {
                qint64 len = getSelectionEnd() - getSelectionBegin();
                replace(getSelectionBegin(), (int)len, QByteArray((int)len, char(0)));
                setCursorPosition(2*_bPosCurrent);
                resetSelection(2*_bPosCurrent);
            }

            /* Change content */
            if (_chunks->size() > 0) {
                QByteArray hexValue = _chunks->data(_bPosCurrent, 1).toHex();

                if ((_cursorPosition % 2) == 0) {
                    hexValue[0] = key;
                } else {
                    hexValue[1] = key;
                }

                replace(_bPosCurrent, QByteArray().fromHex(hexValue)[0]);

                setCursorPosition(_cursorPosition + 1);
                resetSelection(_cursorPosition);
            }
        }
    }

    /* Cut */
    if (e->matches(QKeySequence::Cut)) {
        QByteArray ba = _chunks->data(getSelectionBegin(), getSelectionEnd() - getSelectionBegin()).toHex();

        for (qint64 idx = 32; idx < ba.size(); idx +=33) {
            ba.insert(idx, "\n");
        }

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(ba);
        qint64 len = getSelectionEnd() - getSelectionBegin();
        replace(getSelectionBegin(), (int)len, QByteArray((int)len, char(0)));

        setCursorPosition(2 * getSelectionBegin());
        resetSelection(2 * getSelectionBegin());
    }

    /* Paste */
    if (e->matches(QKeySequence::Paste))
    {
        QClipboard *clipboard = QApplication::clipboard();
        QByteArray ba = QByteArray().fromHex(clipboard->text().toLatin1());
        replace(_bPosCurrent, ba.size(), ba);

        setCursorPosition(_cursorPosition + 2 * ba.size());
        resetSelection(getSelectionBegin());
    }

    /* Delete char */
    if (e->matches(QKeySequence::Delete)) {
        if (getSelectionBegin() != getSelectionEnd()) {
            _bPosCurrent = getSelectionBegin();
            QByteArray ba = QByteArray(getSelectionEnd() - getSelectionBegin(), char(0));
            replace(_bPosCurrent, ba.size(), ba);
        } else {
            replace(_bPosCurrent, char(0));
        }
        setCursorPosition(2 * _bPosCurrent);
        resetSelection(2 * _bPosCurrent);
    }

    /* Backspace */
    if ((e->key() == Qt::Key_Backspace) && (e->modifiers() == Qt::NoModifier)) {
        if (getSelectionBegin() != getSelectionEnd()) {
            _bPosCurrent = getSelectionBegin();
            setCursorPosition(2 * _bPosCurrent);
            QByteArray ba = QByteArray(getSelectionEnd() - getSelectionBegin(), char(0));
            replace(_bPosCurrent, ba.size(), ba);
            resetSelection(2 * _bPosCurrent);
        } else {
            _bPosCurrent -= 1;
            replace(_bPosCurrent, char(0));

            _bPosCurrent -= 1;
            setCursorPosition(2 * _bPosCurrent);
            resetSelection(2 * _bPosCurrent);
        }
    }

    /* undo */
    if (e->matches(QKeySequence::Undo)) {
        undo();
    }

    /* redo */
    if (e->matches(QKeySequence::Redo)) {
        redo();
    }

    /* Copy */
    if (e->matches(QKeySequence::Copy)) {
        QByteArray ba = _chunks->data(getSelectionBegin(), getSelectionEnd() - getSelectionBegin()).toHex();
        for (qint64 idx = 32; idx < ba.size(); idx +=33) {
            ba.insert(idx, "\n");
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(ba);
    }

    refresh();
}

void QHexEdit::mouseMoveEvent(QMouseEvent *e) {
    _blink = false;
    viewport()->update();
    qint64 actPos = cursorPosition(e->pos());
    if (actPos >= 0) {
        setCursorPosition(actPos);
        setSelection(actPos);
        ensureVisible();
    }
}

void QHexEdit::mousePressEvent(QMouseEvent *e) {
    _blink = false;
    viewport()->update();
    qint64 cPos = cursorPosition(e->pos());
    if (cPos >= 0) {
        resetSelection(cPos);
        setCursorPosition(cPos);
    }
}

void QHexEdit::paintEvent(QPaintEvent *e) {
    QPainter painter(viewport());

    /* Process some useful calculations */
    int pxOfsX = horizontalScrollBar()->value();
    int pxPosStartY = _pxCharHeight;

    if (e->rect() != _cursorRect) {
        /* Draw some patterns if needed */
        painter.fillRect(e->rect(), viewport()->palette().color(QPalette::Base));
        if (_asciiArea) {
            int linePos = _pxPosAsciiX;
            painter.setPen(Qt::gray);
            painter.drawLine(linePos - pxOfsX, e->rect().top(), linePos - pxOfsX, height());
        }

        painter.setPen(viewport()->palette().color(QPalette::WindowText));

        /* Paint address area */
        QString address;
        for (int row=0, pxPosY = _pxCharHeight; row <= (_dataShown.size()/bytesPerLine); row++, pxPosY +=_pxCharHeight) {
            address = QString("%1").arg(_bPosFirst + row*bytesPerLine + _addressOffset, _addrDigits, 16, QChar('0'));
            painter.drawText(_pxPosAdrX - pxOfsX, pxPosY, address.toUpper());
        }
        int address_line = _pxPosAdrX - pxOfsX + addressWidth()*_pxCharWidth + _pxCharWidth/2;
        painter.setPen(Qt::gray);
        painter.drawLine(address_line, e->rect().top(), address_line, height());

        /* Paint hex and ASCII area */
        QPen colStandard = QPen(viewport()->palette().color(QPalette::WindowText));

        painter.setBackgroundMode(Qt::TransparentMode);

        for (int row = 0, pxPosY = pxPosStartY; row <= _rowsShown; row++, pxPosY +=_pxCharHeight) {
            QByteArray hex;
            int pxPosX = _pxPosHexX - pxOfsX;
            int pxPosAsciiX2 = _pxPosAsciiX - pxOfsX + _pxCharWidth/2;
            qint64 bPosLine = static_cast<qint64>(row) * static_cast<qint64>(bytesPerLine);
            for (int colIdx = 0; ((bPosLine + colIdx) < _dataShown.size() && (colIdx < bytesPerLine)); colIdx++) {
                QColor c = viewport()->palette().color(QPalette::Base);
                painter.setPen(colStandard);

                qint64 posBa = _bPosFirst + bPosLine + colIdx;
                if ((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa)) {
                    c = _brushSelection.color();
                    painter.setPen(_penSelection);
                } else {
                    if (_highlighting) {
                        if (_markedShown.at((int)(posBa - _bPosFirst))) {
                            c = _brushHighlighted.color();
                            painter.setPen(_penHighlighted);
                        }
                    }
                }

                // render hex value
                QRect r;
                if (colIdx == 0) {
                    r.setRect(pxPosX, pxPosY - _pxCharHeight + _pxSelectionSub, 2*_pxCharWidth, _pxCharHeight);
                } else {
                    r.setRect(pxPosX - _pxCharWidth, pxPosY - _pxCharHeight + _pxSelectionSub, 3*_pxCharWidth, _pxCharHeight);
                }
                painter.fillRect(r, c);
                hex = _hexDataShown.mid((bPosLine + colIdx) * 2, 2);
                painter.drawText(pxPosX, pxPosY, hex.toUpper());
                pxPosX += 3*_pxCharWidth;

                // render ascii value
                if (_asciiArea) {
                    char ch = _dataShown.at(bPosLine + colIdx);
                    if ((ch < 0x20) || (ch > 0x7e)) {
                            ch = '.';
                    }
                    r.setRect(pxPosAsciiX2, pxPosY - _pxCharHeight + _pxSelectionSub, _pxCharWidth, _pxCharHeight);
                    painter.fillRect(r, c);
                    painter.drawText(pxPosAsciiX2, pxPosY, QChar(ch));
                    pxPosAsciiX2 += _pxCharWidth;
                }
            }
        }
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(viewport()->palette().color(QPalette::WindowText));
    }

    // paint cursor
    if (_blink && hasFocus()) {
        painter.fillRect(_cursorRect, this->palette().color(QPalette::WindowText));
    }
}

void QHexEdit::resizeEvent(QResizeEvent *) {
    adjust();
}

/* Selection Handlers */
void QHexEdit::resetSelection() {
    _bSelectionBegin = _bSelectionInit;
    _bSelectionEnd = _bSelectionInit;
}

void QHexEdit::resetSelection(qint64 posa) {
    if (posa < 0) {
        posa = 0;
    }

    posa = posa / 2;
    _bSelectionInit = posa;
    _bSelectionBegin = posa;
    _bSelectionEnd = posa;
}

void QHexEdit::setSelection(qint64 posa) {
    if (posa < 0) {
        posa = 0;
    }

    posa = posa / 2;
    if (posa >= _bSelectionInit) {
        _bSelectionEnd = posa;
        _bSelectionBegin = _bSelectionInit;
    } else {
        _bSelectionBegin = posa;
        _bSelectionEnd = _bSelectionInit;
    }
}

int QHexEdit::getSelectionBegin() {
    return _bSelectionBegin;
}

int QHexEdit::getSelectionEnd() {
    return _bSelectionEnd;
}

/* Private Methods */
void QHexEdit::init() {
    _undoStack->clear();
    setAddressOffset(0);
    resetSelection(0);
    setCursorPosition(0);
    verticalScrollBar()->setValue(0);
    horizontalScrollBar()->setValue(0);
    _modified = false;
}

void QHexEdit::adjust() {
    // recalc Graphics
    if (_addressArea) {
        _addrDigits = addressWidth();
        _pxPosHexX = _pxGapAdr + _addrDigits * _pxCharWidth + _pxGapAdrHex;
    } else {
        _pxPosHexX = _pxGapAdrHex;
    }
    _pxPosAdrX = _pxGapAdr;
    _pxPosAsciiX = (bytesPerLine * 2 * _pxCharWidth) + (bytesPerLine * _pxGapHexAscii) + _pxPosHexX - _pxGapAdr;

    // set horizontalScrollBar()
    int pxWidth = _pxPosAsciiX;
    if (_asciiArea) {
        pxWidth += bytesPerLine * _pxCharWidth;
    }
    horizontalScrollBar()->setRange(0, pxWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());

    // set verticalScrollbar()
    _rowsShown = ((viewport()->height()-4)/_pxCharHeight);
    int lineCount = (int)(_chunks->size() / static_cast<qint64>(bytesPerLine)) + 1;
    verticalScrollBar()->setRange(0, lineCount - _rowsShown);
    verticalScrollBar()->setPageStep(_rowsShown);

    int value = verticalScrollBar()->value();
    _bPosFirst = (qint64)value * bytesPerLine;
    _bPosLast = _bPosFirst + (static_cast<qint64>(_rowsShown) * static_cast<qint64>(bytesPerLine)) - 1;
    if (_bPosLast >= _chunks->size())
        _bPosLast = _chunks->size() - 1;
    readBuffers();
    setCursorPosition(_cursorPosition);
}

int QHexEdit::getLine() {
    return verticalScrollBar()->value();
}

void QHexEdit::setLine(int line) {
    verticalScrollBar()->setValue(line);
}

void QHexEdit::dataChangedPrivate(int) {
     _modified = _undoStack->index() != 0;
    adjust();
    emit dataChanged();
}

void QHexEdit::refresh() {
    ensureVisible();
    readBuffers();
}

void QHexEdit::readBuffers() {
    _dataShown = _chunks->data(_bPosFirst, _bPosLast - _bPosFirst + bytesPerLine + 1, &_markedShown);
    _hexDataShown = QByteArray(_dataShown.toHex());
}

QString QHexEdit::toReadable(const QByteArray &ba) {
    QString result;

    for (int i=0; i < ba.size(); i += 16) {
        QString addrStr = QString("%1").arg(_addressOffset + i, addressWidth(), 16, QChar('0'));
        QString hexStr;
        QString ascStr;
        for (int j=0; j<16; j++) {
            if ((i + j) < ba.size()) {
                hexStr.append(" ").append(ba.mid(i+j, 1).toHex());
                char ch = ba[i + j];
                if ((ch < 0x20) || (ch > 0x7e))
                        ch = '.';
                ascStr.append(QChar(ch));
            }
        }
        result += addrStr + " " + QString("%1").arg(hexStr, -48) + "  " + QString("%1").arg(ascStr, -17) + "\n";
    }
    return result;
}

void QHexEdit::updateCursor() {
    _blink = !(_blink);
    viewport()->update(_cursorRect);
}

