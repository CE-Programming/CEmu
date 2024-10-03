#ifndef QTKEYPADBRIDGE_H
#define QTKEYPADBRIDGE_H

#include "../corethread.h"
#include "keycode.h"
#include "keymap.h"
class CoreWindow;

#include <QtCore/QObject>
#include <QtGui/QKeyEvent>

class QtKeypadBridge : public QObject
{
    Q_OBJECT

public:
    explicit QtKeypadBridge(CoreWindow *parent);

    void skEvent(QKeyEvent *event, bool press);
    void kEvent(QString text, int key = 0, bool repeat = false);
    void releaseAll();
    bool keymapExport(const QString &path);
    bool keymapImport(const QString &path);
    bool eventFilter(QObject *obj, QEvent *e);

public slots:
    void setDev(cemucore_dev_t dev);
    void setKeymap(Keymap map);

signals:
    void keyStateChanged(KeyCode, bool, bool = false);
    void sendKeys(quint16, quint16 = 0, bool = false);

private:
    void updateKeymap();
    QString toModifierString(Qt::KeyboardModifiers m);
    Qt::KeyboardModifiers toModifierValue(QString m);

    CoreWindow *parent() const;

    cemucore_dev_t mDev = cemucore_dev_t::CEMUCORE_DEV_TI84PCE;
    Keymap mKeymap = Keymap::CEmu;
    QHash<quint32, KeyCode> pressed;
    const HostKey *const *keymap = nullptr;

    static const QHash<QChar, quint32> kTextMap;
    static const QHash<int, quint32> kKeyMap;
};

#endif
