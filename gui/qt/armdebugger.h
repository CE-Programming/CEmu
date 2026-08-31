#ifndef ARMDEBUGGER_H
#define ARMDEBUGGER_H

#include "armgdbclient.h"

#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtWidgets/QDialog>

class QCheckBox;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class ArmDebugger : public QDialog {
    Q_OBJECT

public:
    struct PeripheralRegister {
        const char *peripheral;
        const char *name;
        quint32 address;
        quint8 width;
        bool writable;
        bool automatic;
        const char *description;
    };

    explicit ArmDebugger(QWidget *parent = nullptr);
    void connectToServer(quint16 port);
    [[nodiscard]] bool isConnectedToServer(quint16 port) const;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum PeripheralRole {
        AddressRole = Qt::UserRole,
        WidthRole,
        WritableRole,
        AutomaticRole,
    };

    void buildUi();
    void populatePeripherals();
    void setTargetRunning(bool running);
    void refreshAll();
    void refreshPeripherals(bool selectedOnly = false);
    void updatePeripheralFilter(const QString &filter);
    void writePeripheral(QTreeWidgetItem *item);
    void readMemoryView();
    void writeMemoryView();
    void addBreakpoint();
    void removeBreakpoint();
    static bool parseValue(const QString &text, quint32 &value);
    static QString formatValue(quint32 value, unsigned int width);
    static quint32 littleEndianValue(const QByteArray &data);
    static QByteArray littleEndianBytes(quint32 value, unsigned int width);

    ArmGdbClient m_client;
    QTimer m_refreshTimer;
    QLabel *m_status = nullptr;
    QPushButton *m_connect = nullptr;
    QPushButton *m_halt = nullptr;
    QPushButton *m_continue = nullptr;
    QPushButton *m_step = nullptr;
    QPushButton *m_reset = nullptr;
    QPushButton *m_refresh = nullptr;
    QTableWidget *m_registers = nullptr;
    QTableWidget *m_breakpoints = nullptr;
    QTreeWidget *m_peripherals = nullptr;
    QLineEdit *m_filter = nullptr;
    QCheckBox *m_autoRefresh = nullptr;
    QLineEdit *m_memoryAddress = nullptr;
    QSpinBox *m_memorySize = nullptr;
    QPlainTextEdit *m_memoryData = nullptr;
    QHash<quint32, QTreeWidgetItem *> m_peripheralItems;
    quint32 m_memoryRequestAddress = 0;
    quint16 m_serverPort = 0;
    bool m_updating = false;
};

#endif
