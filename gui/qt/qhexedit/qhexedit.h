#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QtWidgets/QAbstractScrollArea>
#include <QtGui/QPen>
#include <QtGui/QBrush>

#include "chunks.h"
#include "commands.h"

class QHexEdit : public QAbstractScrollArea {
    Q_OBJECT

    Q_PROPERTY(bool addressArea READ addressArea WRITE setAddressArea)
    Q_PROPERTY(qint64 addressOffset READ addressOffset WRITE setAddressOffset)
    Q_PROPERTY(bool asciiArea READ asciiArea WRITE setAsciiArea)
    Q_PROPERTY(qint64 cursorPosition READ cursorPosition WRITE setCursorPosition)
    Q_PROPERTY(QByteArray data READ data WRITE setData NOTIFY dataChanged)
    Q_PROPERTY(bool highlighting READ highlighting WRITE setHighlighting)
    Q_PROPERTY(QColor highlightingColor READ highlightingColor WRITE setHighlightingColor)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor)
    Q_PROPERTY(QFont font READ font WRITE setFont)

public:

    explicit QHexEdit(QWidget *parent = Q_NULLPTR);

    bool setData(QIODevice &iODevice);
    QByteArray dataAt(qint64 pos, qint64 count=-1);
    bool write(QIODevice &iODevice, qint64 pos=0, qint64 count=-1);

    // Char handling
    void insert(qint64 pos, char ch);
    void remove(qint64 pos, qint64 len=1);
    void replace(qint64 pos, char ch);


    // ByteArray handling
    void insert(qint64 pos, const QByteArray &ba);
    void replace(qint64 pos, qint64 len, const QByteArray &ba);


    // Utility functioins
    qint64 cursorPosition(QPoint point);
    void ensureVisible();
    qint64 indexOf(const QByteArray &ba, qint64 from);
    bool isModified();
    qint64 lastIndexOf(const QByteArray &ba, qint64 from);
    QString selectionToReadableString();
    virtual void setFont(const QFont &font);
    QString toReadableString();

public slots:
    void redo();
    void undo();

signals:
    /* Contains the address, where the cursor is located. */
    void currentAddressChanged(qint64 address);
    void currentSizeChanged(qint64 size);
    void dataChanged();

public:
    ~QHexEdit();

    void setBytesPerLine(int bytes);

    // Properties
    bool addressArea();
    void setAddressArea(bool addressArea);

    qint64 addressOffset();
    void setAddressOffset(qint64 addressArea);

    int addressWidth();

    bool asciiArea();
    void setAsciiArea(bool asciiArea);

    qint64 cursorPosition();
    void setCursorPosition(qint64 position);

    QByteArray data();
    void setData(const QByteArray &ba);

    bool highlighting();
    void setHighlighting(bool mode);

    QColor highlightingColor();
    void setHighlightingColor(const QColor &color);

    QColor selectionColor();
    void setSelectionColor(const QColor &color);

    void setLine(int line);
    int getLine();

protected:
    // Handle events
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *);

private:
    // Handle selections
    void resetSelection(qint64 pos);            // set selectionStart and selectionEnd to pos
    void resetSelection();                      // set selectionEnd to selectionStart
    void setSelection(qint64 pos);              // set min (if below init) or max (if greater init)
    int getSelectionBegin();
    int getSelectionEnd();

    // Private utility functions
    void init();
    void readBuffers();
    QString toReadable(const QByteArray &ba);

private slots:
    void adjust();                              // recalc pixel positions
    void dataChangedPrivate(int idx=0);        // emit dataChanged() signal
    void refresh();                             // ensureVisible() and readBuffers()
    void updateCursor();                        // update blinking cursor

private:
    // Name convention: pixel positions start with _px
    int _pxCharWidth, _pxCharHeight;            // char dimensions (dpendend on font)
    int _pxPosHexX;                             // X-Pos of HeaxArea
    int _pxPosAdrX;                             // X-Pos of Address Area
    int _pxPosAsciiX;                           // X-Pos of Ascii Area
    int _pxGapAdr;                              // gap left from AddressArea
    int _pxGapAdrHex;                           // gap between AddressArea and HexAerea
    int _pxGapHexAscii;                         // gap between HexArea and AsciiArea
    int _pxCursorWidth;                         // cursor width
    int _pxSelectionSub;                        // offset selection rect
    int _pxCursorX;                             // current cursor pos
    int _pxCursorY;                             // current cursor pos

    // Name convention: absolute byte positions in chunks start with _b
    qint64 _bSelectionBegin;                    // first position of Selection
    qint64 _bSelectionEnd;                      // end of Selection
    qint64 _bSelectionInit;                     // memory position of Selection
    qint64 _bPosFirst;                          // position of first byte shown
    qint64 _bPosLast;                           // position of last byte shown
    qint64 _bPosCurrent;                        // current position

    // variables to store the property values
    bool _addressArea;                          // left area of QHexEdit
    int _addressWidth;
    bool _asciiArea;
    qint64 _addressOffset;
    bool _highlighting;
    bool _overwriteMode;
    QBrush _brushSelection;
    QPen _penSelection;
    QBrush _brushHighlighted;
    QPen _penHighlighted;
    bool _readOnly;

    // other variables
    int _addrDigits;                            // real no of addressdigits, may be > addressWidth
    bool _blink;                                // help get cursor blinking
    QBuffer _bData;                             // buffer, when setup with QByteArray
    Chunks *_chunks;                            // IODevice based access to data
    QTimer _cursorTimer;                        // for blinking cursor
    qint64 _cursorPosition;                     // absolute positioin of cursor, 1 Byte == 2 tics
    QRect _cursorRect;                          // physical dimensions of cursor
    QByteArray _data;                           // QHexEdit's data, when setup with QByteArray
    QByteArray _dataShown;                      // data in the current View
    QByteArray _hexDataShown;                   // data in view, transformed to hex
    qint64 _lastEventSize;                      // size, which was emitted last time
    QByteArray _markedShown;                    // marked data in view
    bool _modified;                             // Is any data in editor modified?
    int _rowsShown;                             // lines of text shown
    UndoStack * _undoStack;                     // Stack to store edit actions for undo/redo
    int bytesPerLine = 8;
};

#endif
