#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QAbstractScrollArea>
#include <QPen>
#include <QBrush>

#include "chunks.h"
#include "commands.h"

class QHexEdit : public QAbstractScrollArea {
    Q_OBJECT

    /*! Property address area switch the address area on or off. Set addressArea true
    (show it), false (hide it).
    */
    Q_PROPERTY(bool addressArea READ addressArea WRITE setAddressArea)

    /*! Property address area color sets (setAddressAreaColor()) the backgorund
    color of address areas. You can also read the color (addressaAreaColor()).
    */
    Q_PROPERTY(QColor addressAreaColor READ addressAreaColor WRITE setAddressAreaColor)

    /*! Property addressOffset is added to the Numbers of the Address Area.
    A offset in the address area (left side) is sometimes usefull, whe you show
    only a segment of a complete memory picture. With setAddressOffset() you set
    this property - with addressOffset() you get the current value.
    */
    Q_PROPERTY(qint64 addressOffset READ addressOffset WRITE setAddressOffset)

    /*! Switch the ascii area on (true, show it) or off (false, hide it).
    */
    Q_PROPERTY(bool asciiArea READ asciiArea WRITE setAsciiArea)

    /*! Porperty cursorPosition sets or gets the position of the editor cursor
    in QHexEdit. Every byte in data has two cursor positions: the lower and upper
    Nibble. Maximum cursor position is factor two of data.size().
    */
    Q_PROPERTY(qint64 cursorPosition READ cursorPosition WRITE setCursorPosition)

    /*! Property data holds the content of QHexEdit. Call setData() to set the
    content of QHexEdit, data() returns the actual content. When calling setData()
    with a QByteArray as argument, QHexEdit creates a internal copy of the data
    If you want to edit big files please use setData(), based on QIODevice.
    */
    Q_PROPERTY(QByteArray data READ data WRITE setData NOTIFY dataChanged)

    /*! Switch the highlighting feature on or of: true (show it), false (hide it).
    */
    Q_PROPERTY(bool highlighting READ highlighting WRITE setHighlighting)

    /*! Property highlighting color sets (setHighlightingColor()) the backgorund
    color of highlighted text areas. You can also read the color
    (highlightingColor()).
    */
    Q_PROPERTY(QColor highlightingColor READ highlightingColor WRITE setHighlightingColor)

    /*! Property selection color sets (setSelectionColor()) the backgorund
    color of selected text areas. You can also read the color
    (selectionColor()).
    */
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor)

    /*! Set the font of the widget. Please use fixed width fonts like Mono or Courier.*/
    Q_PROPERTY(QFont font READ font WRITE setFont)

public:
    /*! Creates an instance of QHexEdit.
    \param parent Parent widget of QHexEdit.
    */
    QHexEdit(QWidget *parent=0);

    // Access to data of qhexedit

    /*! Sets the data of QHexEdit. The QIODevice will be opend just before reading
    and closed immediately afterwards. This is to allow other programs to rewrite
    the file while editing it.
    */
    bool setData(QIODevice &iODevice);

    /*! Givs back the data as a QByteArray starting at position \param pos and
    delivering \param count bytes.
    */
    QByteArray dataAt(qint64 pos, qint64 count=-1);

    /*! Givs back the data into a \param iODevice starting at position \param pos 
    and delivering \param count bytes. 
    */
    bool write(QIODevice &iODevice, qint64 pos=0, qint64 count=-1);


    // Char handling

    /*! Inserts a char.
    \param pos Index position, where to insert
    \param ch Char, which is to insert
    The char will be inserted and size of data grows.
    */
    void insert(qint64 pos, char ch);

    /*! Removes len bytes from the content.
    \param pos Index position, where to remove
    \param len Amount of bytes to remove
    */
    void remove(qint64 pos, qint64 len=1);

    /*! Replaces a char.
    \param pos Index position, where to overwrite
    \param ch Char, which is to insert
    The char will be overwritten and size remains constant.
    */
    void replace(qint64 pos, char ch);


    // ByteArray handling

    /*! Inserts a byte array.
    \param pos Index position, where to insert
    \param ba QByteArray, which is to insert
    The QByteArray will be inserted and size of data grows.
    */
    void insert(qint64 pos, const QByteArray &ba);

    /*! Replaces \param len bytes with a byte array \param ba
    \param pos Index position, where to overwrite
    \param ba QByteArray, which is inserted
    \param len count of bytes to overwrite
    The data is overwritten and size of data may change.
    */
    void replace(qint64 pos, qint64 len, const QByteArray &ba);


    // Utility functioins
    /*! Calc cursor position from graphics position
     * \param point from where the cursor position should be calculated
     * \return Cursor postioin
     */
    qint64 cursorPosition(QPoint point);

    /*! Ensure the cursor to be visble
     */
    void ensureVisible();

    /*! Find first occurence of ba in QHexEdit data
     * \param ba Data to find
     * \param from Point where the search starts
     * \return pos if fond, else -1
     */
    qint64 indexOf(const QByteArray &ba, qint64 from);

    /*! Returns if any changes where done on document
     * \return true when document is modified else false
     */
    bool isModified();

    /*! Find last occurence of ba in QHexEdit data
     * \param ba Data to find
     * \param from Point where the search starts
     * \return pos if fond, else -1
     */
    qint64 lastIndexOf(const QByteArray &ba, qint64 from);

    /*! Gives back a formatted image of the selected content of QHexEdit
    */
    QString selectionToReadableString();

    /*! Set Font of QHexEdit
     * \param font
     */
    virtual void setFont(const QFont &font);

    /*! Gives back a formatted image of the content of QHexEdit
    */
    QString toReadableString();


public slots:
    /*! Redoes the last operation. If there is no operation to redo, i.e.
      there is no redo step in the undo/redo history, nothing happens.
      */
    void redo();

    /*! Undoes the last operation. If there is no operation to undo, i.e.
      there is no undo step in the undo/redo history, nothing happens.
      */
    void undo();

signals:

    /*! Contains the address, where the cursor is located. */
    void currentAddressChanged(qint64 address);

    /*! Contains the size of the data to edit. */
    void currentSizeChanged(qint64 size);

    /*! The signal is emitted every time, the data is changed. */
    void dataChanged();

    /*! The signal is emitted every time, the overwrite mode is changed. */
    void overwriteModeChanged(bool state);


/*! \cond docNever */
public:
    ~QHexEdit();

    // Properties
    bool addressArea();
    void setAddressArea(bool addressArea);

    QColor addressAreaColor();
    void setAddressAreaColor(const QColor &color);

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
    QColor _addressAreaColor;
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
    /*! \endcond docNever */
};

#endif // QHEXEDIT_H
