#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include <QtGui/QKeyEvent>

#include "keycode.h"
#include "keymap.h"

/* This class is used by every Widget which wants to interact with the
 * virtual keypad. Simply call QtKeypadBridge::keyEvent
 * to relay the key events into the virtual calc. */

class QtKeypadBridge : public QObject {
    Q_OBJECT

public:

    typedef enum {
        KEYMAP_NATURAL,
        KEYMAP_CEMU,
        KEYMAP_TILEM,
        KEYMAP_WABBITEMU,
        KEYMAP_JSTIFIED,
        KEYMAP_SMARTPAD,
        KEYMAP_CUSTOM,
    } KeymapMode;

    explicit QtKeypadBridge(QObject *parent = Q_NULLPTR) : QObject(parent) {}

    bool setKeymap(KeymapMode map);
    void skEvent(QKeyEvent *event, bool press);
    void kEvent(QString text, int key = 0, bool repeat = false);
    void releaseAll();
    bool keymapExport(const QString &path);
    bool keymapImport(const QString &path);
    bool eventFilter(QObject *obj, QEvent *e);

signals:
    void keyStateChanged(KeyCode, bool, bool = false);
    void sendKeys(quint16, quint16 = 0, bool = false);

private:
    QString toModifierString(Qt::KeyboardModifiers m);
    Qt::KeyboardModifiers toModifierValue(QString m);

    QHash<quint32, KeyCode> pressed;
    const HostKey *const *keymap = nullptr;
    KeymapMode m_mode;

    static const QHash<QChar, quint32> kTextMap;
    static const QHash<int, quint32> kKeyMap;
};

// global event filter
extern QtKeypadBridge *keypadBridge;

#endif
