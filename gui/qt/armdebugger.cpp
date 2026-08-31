#include "armdebugger.h"

#include <QtCore/QRegularExpression>
#include <QtGui/QCloseEvent>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include <array>
#include <limits>

namespace {

using Register = ArmDebugger::PeripheralRegister;

constexpr std::array<Register, 111> PeripheralRegisters = {{
    {"PM", "SLEEP",       0x40000401, 1, true,  true,  "Sleep mode"},
    {"PM", "CPUSEL",      0x40000408, 1, true,  true,  "CPU clock prescaler"},
    {"PM", "APBASEL",     0x40000409, 1, true,  true,  "APBA clock prescaler"},
    {"PM", "APBBSEL",     0x4000040A, 1, true,  true,  "APBB clock prescaler"},
    {"PM", "APBCSEL",     0x4000040B, 1, true,  true,  "APBC clock prescaler"},
    {"PM", "AHBMASK",     0x40000414, 4, true,  true,  "AHB clock mask"},
    {"PM", "APBAMASK",    0x40000418, 4, true,  true,  "APBA clock mask"},
    {"PM", "APBBMASK",    0x4000041C, 4, true,  true,  "APBB clock mask"},
    {"PM", "APBCMASK",    0x40000420, 4, true,  true,  "APBC clock mask"},
    {"PM", "INTENCLR",    0x40000434, 1, true,  true,  "Interrupt enable clear"},
    {"PM", "INTENSET",    0x40000435, 1, true,  true,  "Interrupt enable set"},
    {"PM", "INTFLAG",     0x40000436, 1, true,  true,  "Interrupt flags (write one to clear)"},
    {"PM", "RCAUSE",      0x40000438, 1, false, true,  "Reset cause"},

    {"SYSCTRL", "PCLKSR",    0x4000080C, 4, false, true, "Power and clock status (modeled ready state)"},
    {"SYSCTRL", "XOSC",      0x40000810, 2, false, true, "External oscillator (reset-value stub)"},
    {"SYSCTRL", "XOSC32K",   0x40000814, 2, false, true, "32 kHz external oscillator (reset-value stub)"},
    {"SYSCTRL", "OSC32K",    0x40000818, 4, false, true, "32 kHz oscillator (reset-value stub)"},
    {"SYSCTRL", "OSCULP32K", 0x4000081C, 1, false, true, "Ultra-low-power oscillator (reset-value stub)"},
    {"SYSCTRL", "OSC8M",     0x40000820, 4, false, true, "8 MHz oscillator (reset-value stub)"},
    {"SYSCTRL", "DFLLCTRL",  0x40000824, 2, false, true, "DFLL control (reset-value stub)"},
    {"SYSCTRL", "DFLLVAL",   0x40000828, 4, false, true, "DFLL value (reset-value stub)"},
    {"SYSCTRL", "DFLLMUL",   0x4000082C, 4, false, true, "DFLL multiplier (reset-value stub)"},
    {"SYSCTRL", "BOD33",     0x40000834, 4, false, true, "Brown-out detector (reset-value stub)"},
    {"SYSCTRL", "VREG",      0x4000083C, 2, false, true, "Voltage regulator (reset-value stub)"},
    {"SYSCTRL", "DPLLCTRLA", 0x40000844, 1, false, true, "DPLL control A (reset-value stub)"},
    {"SYSCTRL", "DPLLRATIO", 0x40000848, 4, false, true, "DPLL ratio (reset-value stub)"},
    {"SYSCTRL", "DPLLCTRLB", 0x4000084C, 4, false, true, "DPLL control B (reset-value stub)"},
    {"SYSCTRL", "DPLLSTATUS",0x40000850, 1, false, true, "DPLL status (reset-value stub)"},

    {"GCLK", "CTRL",    0x40000C00, 1, true,  true, "Generic clock control"},
    {"GCLK", "STATUS",  0x40000C01, 1, false, true, "Synchronization status"},
    {"GCLK", "CLKCTRL", 0x40000C02, 2, true,  true, "Selected generic clock channel"},
    {"GCLK", "GENCTRL", 0x40000C04, 4, true,  true, "Selected generator control"},
    {"GCLK", "GENDIV",  0x40000C08, 4, true,  true, "Selected generator divider"},

    {"NVMCTRL", "CTRLA",    0x41004000, 2, true,  true, "NVM command and key"},
    {"NVMCTRL", "CTRLB",    0x41004004, 4, true,  true, "NVM configuration"},
    {"NVMCTRL", "PARAM",    0x41004008, 4, false, true, "NVM parameters"},
    {"NVMCTRL", "INTENCLR", 0x4100400C, 1, true,  true, "Interrupt enable clear"},
    {"NVMCTRL", "INTENSET", 0x41004010, 1, true,  true, "Interrupt enable set"},
    {"NVMCTRL", "INTFLAG",  0x41004014, 1, true,  true, "Interrupt flags"},
    {"NVMCTRL", "STATUS",   0x41004018, 2, true,  true, "NVM status"},
    {"NVMCTRL", "ADDR",     0x4100401C, 4, true,  true, "NVM address"},
    {"NVMCTRL", "LOCK",     0x41004020, 2, true,  true, "Region locks"},

    {"SERCOM0", "CTRLA",    0x42000800, 4, true,  true,  "SPI/USART control A"},
    {"SERCOM0", "CTRLB",    0x42000804, 4, true,  true,  "SPI/USART control B"},
    {"SERCOM0", "BAUD",     0x4200080C, 2, true,  true,  "Baud rate"},
    {"SERCOM0", "RXPL",     0x4200080E, 1, true,  true,  "USART receive pulse length"},
    {"SERCOM0", "INTENCLR", 0x42000814, 1, true,  true,  "Interrupt enable clear"},
    {"SERCOM0", "INTENSET", 0x42000816, 1, true,  true,  "Interrupt enable set"},
    {"SERCOM0", "INTFLAG",  0x42000818, 1, true,  true,  "Interrupt flags"},
    {"SERCOM0", "STATUS",   0x4200081A, 2, true,  true,  "SPI/USART status"},
    {"SERCOM0", "SYNCBUSY", 0x4200081C, 4, false, true,  "Synchronization status"},
    {"SERCOM0", "ADDR",     0x42000824, 4, true,  true,  "SPI address"},
    {"SERCOM0", "DATA",     0x42000828, 2, true,  false, "FIFO data; reading consumes input"},

    {"SERCOM1", "CTRLA",    0x42000C00, 4, true,  true,  "SPI/USART control A"},
    {"SERCOM1", "CTRLB",    0x42000C04, 4, true,  true,  "SPI/USART control B"},
    {"SERCOM1", "BAUD",     0x42000C0C, 2, true,  true,  "Baud rate"},
    {"SERCOM1", "RXPL",     0x42000C0E, 1, true,  true,  "USART receive pulse length"},
    {"SERCOM1", "INTENCLR", 0x42000C14, 1, true,  true,  "Interrupt enable clear"},
    {"SERCOM1", "INTENSET", 0x42000C16, 1, true,  true,  "Interrupt enable set"},
    {"SERCOM1", "INTFLAG",  0x42000C18, 1, true,  true,  "Interrupt flags"},
    {"SERCOM1", "STATUS",   0x42000C1A, 2, true,  true,  "SPI/USART status"},
    {"SERCOM1", "SYNCBUSY", 0x42000C1C, 4, false, true,  "Synchronization status"},
    {"SERCOM1", "ADDR",     0x42000C24, 4, true,  true,  "SPI address"},
    {"SERCOM1", "DATA",     0x42000C28, 2, true,  false, "FIFO data; reading consumes input"},

    {"SERCOM2", "CTRLA",    0x42001000, 4, true,  true,  "SPI/USART control A"},
    {"SERCOM2", "CTRLB",    0x42001004, 4, true,  true,  "SPI/USART control B"},
    {"SERCOM2", "BAUD",     0x4200100C, 2, true,  true,  "Baud rate"},
    {"SERCOM2", "RXPL",     0x4200100E, 1, true,  true,  "USART receive pulse length"},
    {"SERCOM2", "INTENCLR", 0x42001014, 1, true,  true,  "Interrupt enable clear"},
    {"SERCOM2", "INTENSET", 0x42001016, 1, true,  true,  "Interrupt enable set"},
    {"SERCOM2", "INTFLAG",  0x42001018, 1, true,  true,  "Interrupt flags"},
    {"SERCOM2", "STATUS",   0x4200101A, 2, true,  true,  "SPI/USART status"},
    {"SERCOM2", "SYNCBUSY", 0x4200101C, 4, false, true,  "Synchronization status"},
    {"SERCOM2", "ADDR",     0x42001024, 4, true,  true,  "SPI address"},
    {"SERCOM2", "DATA",     0x42001028, 2, true,  false, "FIFO data; reading consumes input"},

    {"SERCOM3", "CTRLA",    0x42001400, 4, true,  true,  "SPI/USART control A"},
    {"SERCOM3", "CTRLB",    0x42001404, 4, true,  true,  "SPI/USART control B"},
    {"SERCOM3", "BAUD",     0x4200140C, 2, true,  true,  "Baud rate"},
    {"SERCOM3", "RXPL",     0x4200140E, 1, true,  true,  "USART receive pulse length"},
    {"SERCOM3", "INTENCLR", 0x42001414, 1, true,  true,  "Interrupt enable clear"},
    {"SERCOM3", "INTENSET", 0x42001416, 1, true,  true,  "Interrupt enable set"},
    {"SERCOM3", "INTFLAG",  0x42001418, 1, true,  true,  "Interrupt flags"},
    {"SERCOM3", "STATUS",   0x4200141A, 2, true,  true,  "SPI/USART status"},
    {"SERCOM3", "SYNCBUSY", 0x4200141C, 4, false, true,  "Synchronization status"},
    {"SERCOM3", "ADDR",     0x42001424, 4, true,  true,  "SPI address"},
    {"SERCOM3", "DATA",     0x42001428, 2, true,  false, "FIFO data; reading consumes input"},

    {"SysTick", "CTRL",  0xE000E010, 4, true,  true, "SysTick control and status"},
    {"SysTick", "LOAD",  0xE000E014, 4, true,  true, "Reload value"},
    {"SysTick", "VAL",   0xE000E018, 4, true,  true, "Current value; writing clears it"},
    {"SysTick", "CALIB", 0xE000E01C, 4, false, true, "Calibration value"},

    {"NVIC", "ISER", 0xE000E100, 4, true,  true, "Interrupt set-enable"},
    {"NVIC", "ICER", 0xE000E180, 4, true,  true, "Interrupt clear-enable"},
    {"NVIC", "ISPR", 0xE000E200, 4, true,  true, "Interrupt set-pending"},
    {"NVIC", "ICPR", 0xE000E280, 4, true,  true, "Interrupt clear-pending"},
    {"NVIC", "IPR0", 0xE000E400, 4, true,  true, "Interrupt priorities 0-3"},
    {"NVIC", "IPR1", 0xE000E404, 4, true,  true, "Interrupt priorities 4-7"},
    {"NVIC", "IPR2", 0xE000E408, 4, true,  true, "Interrupt priorities 8-11"},
    {"NVIC", "IPR3", 0xE000E40C, 4, true,  true, "Interrupt priorities 12-15"},
    {"NVIC", "IPR4", 0xE000E410, 4, true,  true, "Interrupt priorities 16-19"},
    {"NVIC", "IPR5", 0xE000E414, 4, true,  true, "Interrupt priorities 20-23"},
    {"NVIC", "IPR6", 0xE000E418, 4, true,  true, "Interrupt priorities 24-27"},
    {"NVIC", "IPR7", 0xE000E41C, 4, true,  true, "Interrupt priorities 28-31"},

    {"SCB", "CPUID", 0xE000ED00, 4, false, true, "Cortex-M0+ identification"},
    {"SCB", "ICSR",  0xE000ED04, 4, true,  true, "Interrupt control and state"},
    {"SCB", "VTOR",  0xE000ED08, 4, true,  true, "Vector table offset"},
    {"SCB", "AIRCR", 0xE000ED0C, 4, true,  true, "Application interrupt/reset control"},
    {"SCB", "SCR",   0xE000ED10, 4, true,  true, "System control"},
    {"SCB", "CCR",   0xE000ED14, 4, false, true, "Configuration and control"},
    {"SCB", "SHPR2", 0xE000ED1C, 4, true,  true, "System handler priority 2"},
    {"SCB", "SHPR3", 0xE000ED20, 4, true,  true, "System handler priority 3"},
    {"SCB", "SHCSR", 0xE000ED24, 4, false, true, "System handler control and state"},
}};

constexpr std::array<const char *, 17> RegisterNames = {{
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
    "r10", "r11", "r12", "sp", "lr", "pc", "xpsr",
}};

} // namespace

ArmDebugger::ArmDebugger(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("ARM Coprocessor Debugger"));
    setWindowFlag(Qt::Window, true);
    resize(980, 700);
    buildUi();
    populatePeripherals();

    m_refreshTimer.setInterval(1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this] {
        if (m_autoRefresh->isChecked() && m_client.isReady() && !m_client.isRunning()) refreshAll();
    });
    m_refreshTimer.start();

    connect(&m_client, &ArmGdbClient::ready, this, [this] {
        m_status->setText(tr("Connected and halted on 127.0.0.1:%1").arg(m_serverPort));
        setTargetRunning(false);
    });
    connect(&m_client, &ArmGdbClient::disconnected, this, [this] {
        m_status->setText(tr("Disconnected"));
        m_connect->setEnabled(true);
        setTargetRunning(true);
    });
    connect(&m_client, &ArmGdbClient::errorOccurred, this,
            [this](const QString &message) { m_status->setText(message); });
    connect(&m_client, &ArmGdbClient::targetRunning, this, [this] {
        m_status->setText(tr("ARM target running"));
        setTargetRunning(true);
    });
    connect(&m_client, &ArmGdbClient::targetStopped, this, [this](unsigned int signal) {
        m_status->setText(tr("ARM target halted (signal %1)").arg(signal));
        setTargetRunning(false);
        refreshAll();
    });
    connect(&m_client, &ArmGdbClient::registersReceived, this,
            [this](const QVector<quint32> &registers) {
        m_updating = true;
        for (qsizetype row = 0; row != registers.size() && row < m_registers->rowCount(); ++row) {
            m_registers->item(row, 1)->setText(formatValue(registers[row], 4));
        }
        m_updating = false;
    });
    connect(&m_client, &ArmGdbClient::memoryReceived, this,
            [this](quint32 address, const QByteArray &data) {
        if (QTreeWidgetItem *item = m_peripheralItems.value(address)) {
            m_updating = true;
            item->setText(3, formatValue(littleEndianValue(data), data.size()));
            m_updating = false;
        }
        if (address == m_memoryRequestAddress) {
            QStringList bytes;
            bytes.reserve(data.size());
            for (const unsigned char value : data) bytes.append(QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')));
            m_memoryData->setPlainText(bytes.join(QLatin1Char(' ')));
        }
    });
    connect(&m_client, &ArmGdbClient::memoryWritten, this,
            [this](quint32 address, bool success) {
        if (!success) m_status->setText(tr("ARM memory write failed"));
        else m_client.readMemory(address, m_peripheralItems.contains(address)
                                 ? m_peripheralItems.value(address)->data(0, WidthRole).toUInt()
                                 : static_cast<quint32>(m_memorySize->value()));
    });
    connect(&m_client, &ArmGdbClient::breakpointChanged, this,
            [this](quint32 address, bool inserted, bool success) {
        if (!success) {
            m_status->setText(tr("Could not update ARM breakpoint at %1").arg(formatValue(address, 4)));
            return;
        }
        if (inserted) {
            for (int row = 0; row != m_breakpoints->rowCount(); ++row) {
                if (m_breakpoints->item(row, 0)->data(Qt::UserRole).toUInt() == address) return;
            }
            const int row = m_breakpoints->rowCount();
            m_breakpoints->insertRow(row);
            auto *item = new QTableWidgetItem(formatValue(address, 4));
            item->setData(Qt::UserRole, address);
            m_breakpoints->setItem(row, 0, item);
        } else {
            for (int row = 0; row != m_breakpoints->rowCount(); ++row) {
                if (m_breakpoints->item(row, 0)->data(Qt::UserRole).toUInt() == address) {
                    m_breakpoints->removeRow(row);
                    break;
                }
            }
        }
    });
}

void ArmDebugger::connectToServer(quint16 port) {
    m_serverPort = port;
    m_status->setText(tr("Connecting to 127.0.0.1:%1...").arg(port));
    m_connect->setEnabled(false);
    m_client.connectToHost(QStringLiteral("127.0.0.1"), port);
}

bool ArmDebugger::isConnectedToServer(quint16 port) const {
    return m_serverPort == port && m_client.isActive();
}

void ArmDebugger::closeEvent(QCloseEvent *event) {
    if (m_client.isRunning()) m_client.interruptTarget();
    m_client.disconnectFromTarget();
    event->accept();
}

void ArmDebugger::buildUi() {
    auto *layout = new QVBoxLayout(this);
    auto *controls = new QHBoxLayout;
    m_connect = new QPushButton(tr("Reconnect"), this);
    m_halt = new QPushButton(tr("Halt"), this);
    m_continue = new QPushButton(tr("Continue"), this);
    m_step = new QPushButton(tr("Step"), this);
    m_reset = new QPushButton(tr("Reset"), this);
    m_refresh = new QPushButton(tr("Refresh"), this);
    m_autoRefresh = new QCheckBox(tr("Auto refresh"), this);
    m_status = new QLabel(tr("Disconnected"), this);
    controls->addWidget(m_connect);
    controls->addWidget(m_halt);
    controls->addWidget(m_continue);
    controls->addWidget(m_step);
    controls->addWidget(m_reset);
    controls->addWidget(m_refresh);
    controls->addWidget(m_autoRefresh);
    controls->addWidget(m_status, 1);
    layout->addLayout(controls);

    auto *tabs = new QTabWidget(this);
    auto *cpuTab = new QWidget(tabs);
    auto *cpuLayout = new QHBoxLayout(cpuTab);
    auto *splitter = new QSplitter(Qt::Horizontal, cpuTab);
    m_registers = new QTableWidget(RegisterNames.size(), 2, splitter);
    m_registers->setHorizontalHeaderLabels({tr("Register"), tr("Value")});
    m_registers->verticalHeader()->hide();
    m_registers->horizontalHeader()->setStretchLastSection(true);
    m_registers->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    for (qsizetype row = 0; row != RegisterNames.size(); ++row) {
        auto *name = new QTableWidgetItem(QString::fromLatin1(RegisterNames[row]));
        name->setFlags(name->flags() & ~Qt::ItemIsEditable);
        m_registers->setItem(row, 0, name);
        m_registers->setItem(row, 1, new QTableWidgetItem(QStringLiteral("--------")));
    }
    connect(m_registers, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        if (m_updating || item->column() != 1 || m_client.isRunning()) return;
        quint32 value = 0;
        if (!parseValue(item->text(), value)) {
            m_status->setText(tr("Invalid ARM register value"));
            m_client.requestRegisters();
            return;
        }
        m_client.writeRegister(static_cast<unsigned int>(item->row()), value);
    });

    auto *right = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(right);
    auto *breakGroup = new QGroupBox(tr("Virtual breakpoints"), right);
    auto *breakLayout = new QVBoxLayout(breakGroup);
    m_breakpoints = new QTableWidget(0, 1, breakGroup);
    m_breakpoints->setHorizontalHeaderLabels({tr("Address")});
    m_breakpoints->horizontalHeader()->setStretchLastSection(true);
    m_breakpoints->verticalHeader()->hide();
    m_breakpoints->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto *breakButtons = new QHBoxLayout;
    auto *addBreak = new QPushButton(tr("Add"), breakGroup);
    auto *removeBreak = new QPushButton(tr("Remove"), breakGroup);
    breakButtons->addWidget(addBreak);
    breakButtons->addWidget(removeBreak);
    breakLayout->addWidget(m_breakpoints);
    breakLayout->addLayout(breakButtons);
    rightLayout->addWidget(breakGroup);

    auto *memoryGroup = new QGroupBox(tr("Memory"), right);
    auto *memoryLayout = new QVBoxLayout(memoryGroup);
    auto *memoryForm = new QFormLayout;
    m_memoryAddress = new QLineEdit(QStringLiteral("0x20000000"), memoryGroup);
    m_memorySize = new QSpinBox(memoryGroup);
    m_memorySize->setRange(1, 256);
    m_memorySize->setValue(32);
    memoryForm->addRow(tr("Address"), m_memoryAddress);
    memoryForm->addRow(tr("Bytes"), m_memorySize);
    m_memoryData = new QPlainTextEdit(memoryGroup);
    m_memoryData->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    auto *memoryButtons = new QHBoxLayout;
    auto *readMemory = new QPushButton(tr("Read"), memoryGroup);
    auto *writeMemory = new QPushButton(tr("Write"), memoryGroup);
    memoryButtons->addWidget(readMemory);
    memoryButtons->addWidget(writeMemory);
    memoryLayout->addLayout(memoryForm);
    memoryLayout->addWidget(m_memoryData);
    memoryLayout->addLayout(memoryButtons);
    rightLayout->addWidget(memoryGroup);
    splitter->addWidget(m_registers);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    cpuLayout->addWidget(splitter);
    tabs->addTab(cpuTab, tr("CPU & Memory"));

    auto *peripheralTab = new QWidget(tabs);
    auto *peripheralLayout = new QVBoxLayout(peripheralTab);
    auto *filterLayout = new QHBoxLayout;
    m_filter = new QLineEdit(peripheralTab);
    m_filter->setPlaceholderText(tr("Find peripheral or register..."));
    auto *readSelected = new QPushButton(tr("Read selected"), peripheralTab);
    filterLayout->addWidget(m_filter, 1);
    filterLayout->addWidget(readSelected);
    m_peripherals = new QTreeWidget(peripheralTab);
    m_peripherals->setColumnCount(6);
    m_peripherals->setHeaderLabels({tr("Register"), tr("Address"), tr("Width"),
                                    tr("Value"), tr("Access"), tr("Description")});
    m_peripherals->setAlternatingRowColors(true);
    m_peripherals->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_peripherals->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_peripherals->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_peripherals->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_peripherals->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_peripherals->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_peripherals->header()->setStretchLastSection(true);
    peripheralLayout->addLayout(filterLayout);
    peripheralLayout->addWidget(m_peripherals);
    tabs->addTab(peripheralTab, tr("Peripherals"));
    layout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    connect(m_connect, &QPushButton::clicked, this, [this] { connectToServer(m_serverPort); });
    connect(m_halt, &QPushButton::clicked, &m_client, &ArmGdbClient::interruptTarget);
    connect(m_continue, &QPushButton::clicked, &m_client, &ArmGdbClient::continueTarget);
    connect(m_step, &QPushButton::clicked, &m_client, &ArmGdbClient::stepTarget);
    connect(m_reset, &QPushButton::clicked, &m_client, &ArmGdbClient::resetTarget);
    connect(m_refresh, &QPushButton::clicked, this, &ArmDebugger::refreshAll);
    connect(addBreak, &QPushButton::clicked, this, &ArmDebugger::addBreakpoint);
    connect(removeBreak, &QPushButton::clicked, this, &ArmDebugger::removeBreakpoint);
    connect(readMemory, &QPushButton::clicked, this, &ArmDebugger::readMemoryView);
    connect(writeMemory, &QPushButton::clicked, this, &ArmDebugger::writeMemoryView);
    connect(readSelected, &QPushButton::clicked, this, [this] { refreshPeripherals(true); });
    connect(m_filter, &QLineEdit::textChanged, this, &ArmDebugger::updatePeripheralFilter);
    connect(m_peripherals, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int column) {
        if (!m_updating && column == 3 && item->parent()) writePeripheral(item);
    });
    connect(m_peripherals, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int column) {
        if (column == 3 && item->parent() && item->data(0, WritableRole).toBool() &&
            m_client.isReady() && !m_client.isRunning()) {
            m_peripherals->editItem(item, column);
        }
    });
}

void ArmDebugger::populatePeripherals() {
    QHash<QString, QTreeWidgetItem *> groups;
    m_updating = true;
    for (const PeripheralRegister &reg : PeripheralRegisters) {
        const QString peripheral = QString::fromLatin1(reg.peripheral);
        QTreeWidgetItem *group = groups.value(peripheral);
        if (!group) {
            group = new QTreeWidgetItem(m_peripherals, {peripheral});
            QFont font = group->font(0);
            font.setBold(true);
            group->setFont(0, font);
            groups.insert(peripheral, group);
        }
        auto *item = new QTreeWidgetItem(group);
        item->setText(0, QString::fromLatin1(reg.name));
        item->setText(1, formatValue(reg.address, 4));
        item->setText(2, QString::number(reg.width));
        item->setText(3, reg.automatic ? QStringLiteral("--------") : tr("<manual read>"));
        item->setText(4, reg.writable ? QStringLiteral("R/W") : QStringLiteral("R"));
        item->setText(5, QString::fromLatin1(reg.description));
        item->setData(0, AddressRole, reg.address);
        item->setData(0, WidthRole, reg.width);
        item->setData(0, WritableRole, reg.writable);
        item->setData(0, AutomaticRole, reg.automatic);
        Qt::ItemFlags flags = item->flags();
        if (reg.writable) flags |= Qt::ItemIsEditable;
        item->setFlags(flags);
        m_peripheralItems.insert(reg.address, item);
    }
    m_updating = false;
    m_peripherals->expandAll();
}

void ArmDebugger::setTargetRunning(bool running) {
    const bool connected = m_client.isReady();
    m_connect->setEnabled(!connected);
    m_halt->setEnabled(connected && running);
    m_continue->setEnabled(connected && !running);
    m_step->setEnabled(connected && !running);
    m_reset->setEnabled(connected && !running);
    m_refresh->setEnabled(connected && !running);
    m_registers->setEditTriggers(connected && !running
                                 ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                 : QAbstractItemView::NoEditTriggers);
    m_peripherals->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void ArmDebugger::refreshAll() {
    if (!m_client.isReady() || m_client.isRunning()) return;
    m_client.requestRegisters();
    refreshPeripherals();
}

void ArmDebugger::refreshPeripherals(bool selectedOnly) {
    if (!m_client.isReady() || m_client.isRunning()) return;
    if (selectedOnly) {
        QTreeWidgetItem *item = m_peripherals->currentItem();
        if (item && item->parent()) {
            m_client.readMemory(item->data(0, AddressRole).toUInt(),
                                item->data(0, WidthRole).toUInt());
        }
        return;
    }
    for (QTreeWidgetItem *item : m_peripheralItems) {
        if (item->data(0, AutomaticRole).toBool() && !item->isHidden() && !item->parent()->isHidden()) {
            m_client.readMemory(item->data(0, AddressRole).toUInt(),
                                item->data(0, WidthRole).toUInt());
        }
    }
}

void ArmDebugger::updatePeripheralFilter(const QString &filter) {
    const QString needle = filter.trimmed();
    QTreeWidgetItem *firstMatch = nullptr;
    int matchCount = 0;
    for (int groupIndex = 0; groupIndex != m_peripherals->topLevelItemCount(); ++groupIndex) {
        QTreeWidgetItem *group = m_peripherals->topLevelItem(groupIndex);
        for (int childIndex = 0; childIndex != group->childCount(); ++childIndex) {
            QTreeWidgetItem *item = group->child(childIndex);
            item->setSelected(false);
            const bool match = !needle.isEmpty() &&
                (group->text(0) + QLatin1Char(' ') + item->text(0) + QLatin1Char(' ') + item->text(5))
                    .contains(needle, Qt::CaseInsensitive);
            if (match) {
                if (!firstMatch) firstMatch = item;
                ++matchCount;
            }
        }
    }
    if (firstMatch) {
        firstMatch->parent()->setExpanded(true);
        m_peripherals->setCurrentItem(firstMatch);
        m_peripherals->scrollToItem(firstMatch, QAbstractItemView::PositionAtCenter);
        m_status->setText(tr("Found %1 matching ARM peripheral register(s)").arg(matchCount));
    } else if (!needle.isEmpty()) {
        m_status->setText(tr("No matching ARM peripheral registers"));
    } else if (m_client.isReady() && !m_client.isRunning()) {
        m_status->setText(tr("Connected and halted on 127.0.0.1:%1").arg(m_serverPort));
    }
}

void ArmDebugger::writePeripheral(QTreeWidgetItem *item) {
    if (!item->data(0, WritableRole).toBool() || m_client.isRunning()) return;
    quint32 value = 0;
    if (!parseValue(item->text(3), value)) {
        m_status->setText(tr("Invalid peripheral register value"));
        refreshPeripherals(true);
        return;
    }
    const quint32 address = item->data(0, AddressRole).toUInt();
    const unsigned int width = item->data(0, WidthRole).toUInt();
    m_client.writeMemory(address, littleEndianBytes(value, width));
}

void ArmDebugger::readMemoryView() {
    quint32 address = 0;
    if (!parseValue(m_memoryAddress->text(), address)) {
        m_status->setText(tr("Invalid ARM memory address"));
        return;
    }
    m_memoryRequestAddress = address;
    m_client.readMemory(address, static_cast<quint32>(m_memorySize->value()));
}

void ArmDebugger::writeMemoryView() {
    quint32 address = 0;
    if (!parseValue(m_memoryAddress->text(), address)) {
        m_status->setText(tr("Invalid ARM memory address"));
        return;
    }
    const QStringList tokens = m_memoryData->toPlainText().split(
        QRegularExpression(QStringLiteral("[\\s,]+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty() || tokens.size() > 256) {
        m_status->setText(tr("Enter between 1 and 256 hexadecimal bytes"));
        return;
    }
    QByteArray data(tokens.size(), Qt::Uninitialized);
    for (qsizetype index = 0; index != tokens.size(); ++index) {
        bool valid = false;
        const uint value = tokens[index].toUInt(&valid, 16);
        if (!valid || value > 0xFF) {
            m_status->setText(tr("Invalid hexadecimal byte: %1").arg(tokens[index]));
            return;
        }
        data[index] = static_cast<char>(value);
    }
    m_memoryRequestAddress = address;
    m_memorySize->setValue(data.size());
    m_client.writeMemory(address, data);
}

void ArmDebugger::addBreakpoint() {
    bool accepted = false;
    const QString text = QInputDialog::getText(this, tr("Add ARM breakpoint"), tr("Address"),
                                               QLineEdit::Normal, QStringLiteral("0x00000000"), &accepted);
    quint32 address = 0;
    if (accepted && parseValue(text, address)) m_client.addBreakpoint(address);
}

void ArmDebugger::removeBreakpoint() {
    const int row = m_breakpoints->currentRow();
    if (row >= 0) m_client.removeBreakpoint(m_breakpoints->item(row, 0)->data(Qt::UserRole).toUInt());
}

bool ArmDebugger::parseValue(const QString &text, quint32 &value) {
    QString input = text.trimmed();
    int base = 10;
    if (input.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        input.remove(0, 2);
        base = 16;
    }
    bool valid = false;
    const qulonglong parsed = input.toULongLong(&valid, base);
    if (!valid || parsed > (std::numeric_limits<quint32>::max)()) return false;
    value = static_cast<quint32>(parsed);
    return true;
}

QString ArmDebugger::formatValue(quint32 value, unsigned int width) {
    return QStringLiteral("0x%1").arg(value, static_cast<int>(width * 2), 16, QLatin1Char('0'));
}

quint32 ArmDebugger::littleEndianValue(const QByteArray &data) {
    quint32 value = 0;
    for (qsizetype index = 0; index != data.size() && index != 4; ++index) {
        value |= static_cast<quint32>(static_cast<unsigned char>(data[index])) << (index * 8);
    }
    return value;
}

QByteArray ArmDebugger::littleEndianBytes(quint32 value, unsigned int width) {
    QByteArray data(width, Qt::Uninitialized);
    for (unsigned int index = 0; index != width; ++index) {
        data[index] = static_cast<char>(value >> (index * 8));
    }
    return data;
}
