#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dockwidget.h"
#include "utils.h"
#include "visualizerwidget.h"
#include "tivarslib/TIVarType.h"
#include "tivarslib/TypeHandlers/TypeHandlers.h"
#include "../../core/asic.h"
#include "../../core/cpu.h"
#include "../../core/misc.h"
#include "../../core/mem.h"
#include "../../core/cert.h"
#include "../../core/interrupt.h"
#include "../../core/keypad.h"
#include "../../core/control.h"
#include "../../core/flash.h"
#include "../../core/lcd.h"
#include "../../core/spi.h"
#include "../../core/backlight.h"
#include "../../core/timers.h"
#include "../../core/usb.h"
#include "../../core/realclock.h"
#include "../../core/sha256.h"
#include "../../core/schedule.h"

#include <QtWidgets/QToolTip>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QDesktopWidget>
#include <QtNetwork/QNetworkReply>
#include <QtGui/QClipboard>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

const QString MainWindow::DEBUG_UNSET_ADDR = QStringLiteral("XXXXXX");
const QString MainWindow::DEBUG_UNSET_PORT = QStringLiteral("XXXX");

// -----------------------------------------------
// Debugger Initialization
// -----------------------------------------------

void MainWindow::debugInit() {
    disasmInit();

    ui->afregView->installEventFilter(this);
    ui->hlregView->installEventFilter(this);
    ui->bcregView->installEventFilter(this);
    ui->deregView->installEventFilter(this);
    ui->ixregView->installEventFilter(this);
    ui->iyregView->installEventFilter(this);
    ui->af_regView->installEventFilter(this);
    ui->hl_regView->installEventFilter(this);
    ui->bc_regView->installEventFilter(this);
    ui->de_regView->installEventFilter(this);
    ui->rregView->installEventFilter(this);
    ui->hl->installEventFilter(this);
    ui->bc->installEventFilter(this);
    ui->de->installEventFilter(this);
    ui->ix->installEventFilter(this);
    ui->iy->installEventFilter(this);
    ui->hl_->installEventFilter(this);
    ui->bc_->installEventFilter(this);
    ui->de_->installEventFilter(this);
    ui->pc->installEventFilter(this);
    ui->spl->installEventFilter(this);

    ui->checkADLDisasm->blockSignals(true);
    ui->checkADLDisasm->setCheckState(Qt::PartiallyChecked);
    ui->checkADLDisasm->blockSignals(false);
    ui->checkADLStack->blockSignals(true);
    ui->checkADLStack->setCheckState(Qt::PartiallyChecked);
    ui->checkADLStack->blockSignals(false);

    debug_init();
}

// ------------------------------------------------
// Main Debugger things
// ------------------------------------------------

void MainWindow::debugDisable() {
    guiDebug = false;
    debugGuiState(false);
}

void MainWindow::debugEnable() {
    guiDebug = true;
    debugGuiState(true);
}

void MainWindow::debugStep(int mode) {
    if (mode == DBG_RUN_UNTIL) {
        debug_step(mode, m_runUntilAddr);
    } else {
        disasm.base = cpu.registers.PC;
        disasm.ctx.zdis_adl = cpu.ADL;
        disasmGet();
        debug_step(mode, disasm.next);
    }
    emu.resume();
}

void MainWindow::debugImportFile(const QString &file) {
    if (file.isEmpty()) {
        return;
    }
    int i;

    // check the debug file version
    QSettings info(file, QSettings::IniFormat);
    if (info.value(QStringLiteral("version")) != VERSION_DBG) {
error:
        QMessageBox *warn = new QMessageBox;
        warn->setWindowTitle(tr("Invalid Configuration"));
        warn->setText(tr("The debugging configuration is incompatible with CEmu"));
        warn->setWindowModality(Qt::ApplicationModal);
        warn->setAttribute(Qt::WA_DeleteOnClose);
        warn->show();
        return;
    }

    // load the breakpoint information
    QStringList breakLabel = info.value(QStringLiteral("breakpoints/label")).toStringList();
    QStringList breakAddr = info.value(QStringLiteral("breakpoints/address")).toStringList();
    QStringList breakSet = info.value(QStringLiteral("breakpoints/enable")).toStringList();
    if ((breakLabel.size() + breakAddr.size() + breakSet.size()) / 3 != breakLabel.size()) {
        goto error;
    }
    for (i = 0; i < breakLabel.size(); i++) {
        breakAdd(breakLabel.at(i), hex2int(breakAddr.at(i)), breakSet.at(i) == TXT_YES, false, false);
    }

    // load the watchpoint information
    QStringList watchLabel = info.value(QStringLiteral("watchpoints/label")).toStringList();
    QStringList watchLow = info.value(QStringLiteral("watchpoints/low")).toStringList();
    QStringList watchHigh = info.value(QStringLiteral("watchpoints/high")).toStringList();
    QStringList watchR = info.value(QStringLiteral("watchpoints/read")).toStringList();
    QStringList watchW = info.value(QStringLiteral("watchpoints/write")).toStringList();
    if ((watchLabel.size() + watchLow.size() + watchHigh.size() + watchR.size() + watchW.size()) / 5 != watchLabel.size()) {
        goto error;
    }
    for (i = 0; i < watchLabel.size(); i++) {
        int mask = (watchR.at(i) == TXT_YES ? DBG_MASK_READ : DBG_MASK_NONE) |
                   (watchW.at(i) == TXT_YES ? DBG_MASK_WRITE : DBG_MASK_NONE);
        watchAdd(watchLabel.at(i), hex2int(watchLow.at(i)), hex2int(watchHigh.at(i)), mask, false, false);
    }

    // load the port monitor information
    QStringList portAddr = info.value(QStringLiteral("portmonitor/address")).toStringList();
    QStringList portR = info.value(QStringLiteral("portmonitor/read")).toStringList();
    QStringList portW = info.value(QStringLiteral("portmonitor/write")).toStringList();
    QStringList portF = info.value(QStringLiteral("portmonitor/freeze")).toStringList();
    if ((portAddr.size() + portR.size() + portW.size() + portF.size()) / 4 != portAddr.size()) {
        goto error;
    }
    for (i = 0; i < portAddr.size(); i++) {
        int mask = (portR.at(i) == TXT_YES ? DBG_MASK_PORT_READ : DBG_MASK_NONE)  |
                   (portW.at(i) == TXT_YES ? DBG_MASK_PORT_WRITE : DBG_MASK_NONE) |
                   (portF.at(i) == TXT_YES ? DBG_MASK_PORT_FREEZE : DBG_MASK_NONE);
        portAdd(hex2int(portAddr.at(i)), mask, false);
    }

    // add all the equate files and load them in
    m_equateFiles = info.value(QStringLiteral("equates/files")).toStringList();

    disasm.map.clear();
    disasm.reverse.clear();
    for (QString &file : m_equateFiles) {
        equatesAddFile(file);
    }
}

void MainWindow::debugExportFile(const QString &filename) {
    if (filename.isEmpty()) {
        return;
    }
    int i;

    QSettings info(filename, QSettings::IniFormat);

    // Set the file format version
    info.setValue(QStringLiteral("version"), VERSION_DBG);

    // Save the breakpoint information
    QStringList breakLabel;
    QStringList breakAddr;
    QStringList breakSet;
    for(i = 0; i < m_breakpoints->rowCount(); i++) {
        if (m_breakpoints->item(i, BREAK_ADDR_LOC)->text() != DEBUG_UNSET_ADDR) {
            breakLabel.append(m_breakpoints->item(i, BREAK_NAME_LOC)->text());
            breakAddr.append(m_breakpoints->item(i, BREAK_ADDR_LOC)->text());
            breakSet.append(static_cast<QAbstractButton *>(m_breakpoints->cellWidget(i, BREAK_ENABLE_LOC))->isChecked() ? TXT_YES : TXT_NO);
        }
    }

    info.setValue(QStringLiteral("breakpoints/label"), breakLabel);
    info.setValue(QStringLiteral("breakpoints/address"), breakAddr);
    info.setValue(QStringLiteral("breakpoints/enable"), breakSet);

    // Save watchpoint information
    QStringList watchLabel;
    QStringList watchLow;
    QStringList watchHigh;
    QStringList watchR;
    QStringList watchW;
    for(i = 0; i < m_watchpoints->rowCount(); i++) {
        if (m_watchpoints->item(i, WATCH_LOW_LOC)->text() != DEBUG_UNSET_ADDR) {
            watchLabel.append(m_watchpoints->item(i, WATCH_NAME_LOC)->text());
            watchLow.append(m_watchpoints->item(i, WATCH_LOW_LOC)->text());
            watchHigh.append(m_watchpoints->item(i, WATCH_HIGH_LOC)->text());
            watchR.append(static_cast<QAbstractButton *>(m_watchpoints->cellWidget(i, WATCH_READ_LOC))->isChecked() ? TXT_YES : TXT_NO);
            watchW.append(static_cast<QAbstractButton *>(m_watchpoints->cellWidget(i, WATCH_WRITE_LOC))->isChecked() ? TXT_YES : TXT_NO);
        }
    }

    info.setValue(QStringLiteral("watchpoints/label"), watchLabel);
    info.setValue(QStringLiteral("watchpoints/low"), watchLow);
    info.setValue(QStringLiteral("watchpoints/high"), watchHigh);
    info.setValue(QStringLiteral("watchpoints/read"), watchR);
    info.setValue(QStringLiteral("watchpoints/write"), watchW);

    // Save port monitor information
    QStringList portAddr;
    QStringList portR;
    QStringList portW;
    QStringList portF;
    for(i = 0; i < m_ports->rowCount(); i++) {
        if (m_ports->item(i, PORT_ADDR_LOC)->text() != DEBUG_UNSET_PORT) {
            portAddr.append(m_ports->item(i, PORT_ADDR_LOC)->text());
            portR.append(static_cast<QAbstractButton *>(m_ports->cellWidget(i, PORT_READ_LOC))->isChecked() ? TXT_YES : TXT_NO);
            portW.append(static_cast<QAbstractButton *>(m_ports->cellWidget(i, PORT_WRITE_LOC))->isChecked() ? TXT_YES : TXT_NO);
            portF.append(static_cast<QAbstractButton *>(m_ports->cellWidget(i, PORT_FREEZE_LOC))->isChecked() ? TXT_YES : TXT_NO);
        }
    }

    info.setValue(QStringLiteral("portmonitor/address"), portAddr);
    info.setValue(QStringLiteral("portmonitor/read"), portR);
    info.setValue(QStringLiteral("portmonitor/write"), portW);
    info.setValue(QStringLiteral("portmonitor/freeze"), portF);

    info.setValue(QStringLiteral("equates/files"), m_equateFiles);
    info.sync();
}

QString MainWindow::debugGetFile(bool save) {
    QString file;
    QFileDialog dialog(this);

    dialog.setAcceptMode(save ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(m_dir);
    dialog.setNameFilter(tr("Debugging Info (*.ini)"));
    dialog.setWindowTitle(save ? tr("Debugger Import") : tr("Debugger Export"));
    dialog.setDefaultSuffix(QStringLiteral("ini"));
    dialog.exec();

    if (!dialog.selectedFiles().isEmpty()) {
        QStringList selected = dialog.selectedFiles();
        file = selected.first();
    }

    m_dir = dialog.directory();
    return file;
}

void MainWindow::debugRaise() {
    debugEnable();
    debugPopulate();
    connect(m_shortcutStepIn, &QShortcut::activated, this, &MainWindow::stepIn);
    connect(m_shortcutStepOver, &QShortcut::activated, this, &MainWindow::stepOver);
    connect(m_shortcutStepNext, &QShortcut::activated, this, &MainWindow::stepNext);
    connect(m_shortcutStepOut, &QShortcut::activated, this, &MainWindow::stepOut);
}

void MainWindow::debugExecute(uint32_t offset, uint8_t cmd) {
    m_useSoftCom = true;

    if (offset == 0xFF) {
        int tmp;
        switch (cmd) {
            case CMD_ABORT:
                ui->debuggerLabel->setText(QStringLiteral("Program Aborted"));
                debugRaise();
                m_useSoftCom = false;
                return; // don't exit the debugger
            case CMD_DEBUG:
                ui->debuggerLabel->setText(QStringLiteral("Program Entered Debugger"));
                debugRaise();
                m_useSoftCom = false;
                return; // don't exit the debugger
            case CMD_SET_BREAKPOINT:
                breakAdd(breakNextLabel(), cpu.registers.DE, true, false, false);
                break;
            case CMD_REM_BREAKPOINT:
                breakRemove(cpu.registers.DE);
                break;
            case CMD_SET_R_WATCHPOINT:
                watchRemove(cpu.registers.DE);
                if (cpu.registers.bc.l > 0) {
                    watchAdd(watchNextLabel(), cpu.registers.DE, cpu.registers.DE + cpu.registers.bc.l - 1, DBG_MASK_READ, false, false);
                }
                break;
            case CMD_SET_W_WATCHPOINT:
                watchRemove(cpu.registers.DE);
                if (cpu.registers.bc.l > 0) {
                    watchAdd(watchNextLabel(), cpu.registers.DE, cpu.registers.DE + cpu.registers.bc.l - 1, DBG_MASK_WRITE, false, false);
                }
                break;
            case CMD_SET_RW_WATCHPOINT:
                watchRemove(cpu.registers.DE);
                if (cpu.registers.bc.l > 0) {
                    watchAdd(watchNextLabel(), cpu.registers.DE, cpu.registers.DE + cpu.registers.bc.l - 1, DBG_MASK_RW, false, false);
                }
                break;
            case CMD_REM_WATCHPOINT:
                watchRemove(cpu.registers.DE);
                break;
            case CMD_REM_ALL_BREAK:
                tmp = m_breakpoints->rowCount();
                for (int i = 0; i < tmp; i++) {
                    breakRemoveRow(0);
                }
                break;
            case CMD_REM_ALL_WATCH:
                tmp = m_watchpoints->rowCount();
                for (int i = 0; i < tmp; i++) {
                    watchRemoveRow(0);
                }
                break;
            case CMD_SET_E_WATCHPOINT:
                watchRemove(cpu.registers.DE);
                if (cpu.registers.bc.l > 0) {
                    watchAdd(watchNextLabel(), cpu.registers.DE, cpu.registers.bc.l, DBG_MASK_NONE, false, false);
                }
                break;
            default:
                console(QStringLiteral("[CEmu] Unknown debug Command: 0x") +
                        QString::number(cmd, 16).rightJustified(2, '0') +
                        QStringLiteral(",0x") +
                        QString::number(offset + 0xFFFF00, 16) +
                        QStringLiteral("\n"), Qt::darkRed);
                break;
        }
    }

    m_useSoftCom = false;

    // continue emulation
    if (guiDebug) {
        debugRaise();
    } else {
        emu.debug(false);
    }
}

void MainWindow::debugCommand(int reason, uint32_t data) {
    if (!guiEmuValid) {
        return;
    }

    if (reason == DBG_READY) {
        guiReset = false;
        emu.resume();
        return;
    }

    if (guiReset) {
        emu.resume();
        return;
    }

    int row = 0;
    uint32_t addr;

    // This means the program is trying to send us a debug command. Let's see what we can do with that information
    if (reason >= DBG_NUMBER) {
        debugExecute(static_cast<uint32_t>(reason - DBG_PORT_RANGE), static_cast<uint8_t>(data));
        return;
    }

    QString input, type, text;
    QString label = QStringLiteral("Unknown");
    bool valid = false;

    switch (reason) {
        case DBG_BREAKPOINT:
            input = int2hex(data, 6);

            for (int i = 0; i < m_breakpoints->rowCount(); i++) {
                if (m_breakpoints->item(i, BREAK_ADDR_LOC)->text() == input) {
                    label = m_breakpoints->item(row, BREAK_NAME_LOC)->text();
                    valid = true;
                    break;
                }
            }
            if (valid == false) {
                emu.resume();
                return;
            }

            text = tr("Hit breakpoint ") + input + QStringLiteral(" (") + label + QStringLiteral(")");
            break;
        case DBG_WATCHPOINT_READ:
        case DBG_WATCHPOINT_WRITE:
            input = int2hex(data, 6);
            addr = static_cast<uint32_t>(hex2int(input));
            type = (reason == DBG_WATCHPOINT_READ) ? tr("read") : tr("write");
            text = tr("Hit ") + type + tr(" watchpoint ") + input;

            for (int i = 0; i < m_watchpoints->rowCount(); i++) {
                uint32_t low = static_cast<uint32_t>(hex2int(m_watchpoints->item(i, WATCH_LOW_LOC)->text()));
                uint32_t high = static_cast<uint32_t>(hex2int(m_watchpoints->item(i, WATCH_HIGH_LOC)->text()));
                if (addr >= low && addr <= high) {
                    label = m_watchpoints->item(row, WATCH_NAME_LOC)->text();
                    valid = true;
                    break;
                }
            }
            if (valid == false) {
                emu.resume();
                return;
            }

            text = tr("Hit ") + type + tr(" watchpoint ") + input + QStringLiteral(" (") + label + QStringLiteral(")");

            gotoMemAddr(static_cast<uint32_t>(hex2int(input)));
            break;
        case DBG_PORT_READ:
        case DBG_PORT_WRITE:
            input = int2hex(data, 4);
            type = (reason == DBG_PORT_READ) ? tr("Read") : tr("Wrote");
            text = type + tr(" port ") + input;
            break;
        case DBG_NMI_TRIGGERED:
            text = tr("NMI triggered");
            break;
        case DBG_WATCHDOG_TIMEOUT:
            text = tr("Watchdog timeout");
            break;
        case DBG_MISC_RESET:
            text = tr("Misc. reset");
            break;
        default:
        case DBG_USER:
            debugRaise();
            return;
    }

    // Checked everything, now we should raise the debugger
    if (!text.isEmpty()) {
        ui->debuggerLabel->setText(text);
    }
    debugRaise();
}

void MainWindow::debugSync() {
    if (!guiDebug) {
        return;
    }

    // Update all the changes in the core
    cpu.registers.AF = static_cast<uint16_t>(hex2int(ui->afregView->text()));
    cpu.registers.HL = static_cast<uint32_t>(hex2int(ui->hlregView->text()));
    cpu.registers.DE = static_cast<uint32_t>(hex2int(ui->deregView->text()));
    cpu.registers.BC = static_cast<uint32_t>(hex2int(ui->bcregView->text()));
    cpu.registers.IX = static_cast<uint32_t>(hex2int(ui->ixregView->text()));
    cpu.registers.IY = static_cast<uint32_t>(hex2int(ui->iyregView->text()));

    cpu.registers._AF = static_cast<uint16_t>(hex2int(ui->af_regView->text()));
    cpu.registers._HL = static_cast<uint32_t>(hex2int(ui->hl_regView->text()));
    cpu.registers._DE = static_cast<uint32_t>(hex2int(ui->de_regView->text()));
    cpu.registers._BC = static_cast<uint32_t>(hex2int(ui->bc_regView->text()));

    cpu.registers.SPL = static_cast<uint32_t>(hex2int(ui->splregView->text()));
    cpu.registers.SPS = static_cast<uint16_t>(hex2int(ui->spsregView->text()));

    cpu.registers.MBASE = static_cast<uint8_t>(hex2int(ui->mbregView->text()));
    cpu.registers.I = static_cast<uint16_t>(hex2int(ui->iregView->text()));
    cpu.registers.R = static_cast<uint8_t>(hex2int(ui->rregView->text()));
    cpu.registers.R = cpu.registers.R << 1 | cpu.registers.R >> 7;
    cpu.IM = static_cast<uint8_t>(hex2int(ui->imregView->text()));
    cpu.IM += !!cpu.IM;

    cpu.registers.flags.Z = ui->checkZ->isChecked();
    cpu.registers.flags.C = ui->checkC->isChecked();
    cpu.registers.flags.H = ui->checkHC->isChecked();
    cpu.registers.flags.PV = ui->checkPV->isChecked();
    cpu.registers.flags.N = ui->checkN->isChecked();
    cpu.registers.flags.S = ui->checkS->isChecked();
    cpu.registers.flags._5 = ui->check5->isChecked();
    cpu.registers.flags._3 = ui->check3->isChecked();

    cpu.halted = ui->checkHalted->isChecked();
    cpu.ADL = ui->checkADL->isChecked();
    cpu.MADL = ui->checkMADL->isChecked();
    cpu.halted = ui->checkHalted->isChecked();
    cpu.IEF1 = ui->checkIEF1->isChecked();
    cpu.IEF2 = ui->checkIEF2->isChecked();

    debug.totalCycles = ui->cycleView->text().toLongLong() + (m_ignoreDmaCycles ? debug.dmaCycles : 0);

    uint32_t uiPC = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
    if (cpu.registers.PC != uiPC) {
        cpu_flush(uiPC, ui->checkADL->isChecked());
    }

    backlight.brightness = static_cast<uint8_t>(ui->brightnessSlider->value());
    backlight.factor = (310 - (float)backlight.brightness) / 160.0;

    lcd.upbase = static_cast<uint32_t>(hex2int(ui->lcdbaseView->text()));
    lcd.upcurr = static_cast<uint32_t>(hex2int(ui->lcdcurrView->text()));

    lcd.control &= ~14;
    lcd.control |= ui->bppView->currentIndex() << 1;

    set_reset(ui->checkPowered->isChecked(), 0x800, lcd.control);
    set_reset(ui->checkBEPO->isChecked(), 0x400, lcd.control);
    set_reset(ui->checkBEBO->isChecked(), 0x200, lcd.control);
    set_reset(ui->checkBGR->isChecked(), 0x100, lcd.control);

    lcd_update();

    ui->debuggerLabel->clear();
}

void MainWindow::debugGuiState(bool state) {
    if (state) {
        ui->buttonRun->setText(tr("Run"));
        ui->buttonRun->setIcon(m_iconRun);
    } else {
        ui->buttonRun->setText(tr("Stop"));
        ui->buttonRun->setIcon(m_iconStop);
        ui->debuggerLabel->clear();
    }

    m_disasm->setEnabled(state);
    ui->buttonGoto->setEnabled(state);
    ui->buttonStepIn->setEnabled(state);
    ui->buttonStepOver->setEnabled(state);
    ui->buttonStepNext->setEnabled(state);
    ui->buttonStepOut->setEnabled(state);
    ui->buttonCertID->setEnabled(state);
    ui->groupCPU->setEnabled(state);
    ui->groupFlags->setEnabled(state);
    ui->groupRegisters->setEnabled(state);
    ui->groupInterrupts->setEnabled(state);
    ui->groupStack->setEnabled(state);
    ui->groupFlash->setEnabled(state);
    ui->groupRAM->setEnabled(state);
    ui->cycleView->setEnabled(state);
    ui->freqView->setEnabled(state);
    ui->groupRTC->setEnabled(state);
    ui->groupGPT->setEnabled(state);
    ui->groupLcdState->setEnabled(state);
    ui->groupLcdRegs->setEnabled(state);
    ui->groupBattery->setEnabled(state);
    ui->groupTrigger->setEnabled(state);

    ui->opView->setEnabled(state && m_normalOs);
    ui->vatView->setEnabled(state && m_normalOs);
    ui->opStack->setEnabled(state && m_normalOs);
    ui->fpStack->setEnabled(state && m_normalOs);

    ui->buttonSend->setEnabled(!state);
    ui->buttonRefreshList->setEnabled(!state);
    ui->emuVarView->setEnabled(!state);
    ui->buttonResendFiles->setEnabled(!state);
    ui->buttonReceiveFiles->setEnabled(!state && guiReceive);
    ui->buttonReceiveFile->setEnabled(!state && guiReceive);

    QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    foreach (QDockWidget* dock, docks) {
        if (dock->windowTitle().contains(TXT_MEM_DOCK)) {
            if (dock->isVisible()) {
                QList<QPushButton*> buttons = dock->findChildren<QPushButton*>();
                QList<QToolButton*> tools = dock->findChildren<QToolButton*>();
                QList<HexWidget*> editChildren = dock->findChildren<HexWidget*>();
                QList<QSpinBox*> spinChildren = dock->findChildren<QSpinBox*>();
                editChildren.first()->setEnabled(state);
                spinChildren.first()->setEnabled(state);
                foreach (QPushButton *button, buttons) {
                    button->setEnabled(state);
                }
                foreach (QToolButton *tool, tools) {
                    tool->setEnabled(state);
                }
            }
        }
    }
}

void MainWindow::debugToggle() {
    bool state = guiDebug;

    if (m_pathRom.isEmpty()) {
        return;
    }

    if (guiReceive) {
        varToggle();
    }

    if (state) {
        debugSync();
        debugDisable();
    }

    emu.debug(!state);
}

void MainWindow::debugPopulate() {
    QString tmp;

    tmp = int2hex(cpu.registers.AF, 4);
    ui->afregView->setPalette(tmp == ui->afregView->text() ? m_cNone : m_cBack);
    ui->afregView->setText(tmp);

    tmp = int2hex(cpu.registers.HL, 6);
    ui->hlregView->setPalette(tmp == ui->hlregView->text() ? m_cNone : m_cBack);
    ui->hlregView->setText(tmp);

    tmp = int2hex(cpu.registers.DE, 6);
    ui->deregView->setPalette(tmp == ui->deregView->text() ? m_cNone : m_cBack);
    ui->deregView->setText(tmp);

    tmp = int2hex(cpu.registers.BC, 6);
    ui->bcregView->setPalette(tmp == ui->bcregView->text() ? m_cNone : m_cBack);
    ui->bcregView->setText(tmp);

    tmp = int2hex(cpu.registers.IX, 6);
    ui->ixregView->setPalette(tmp == ui->ixregView->text() ? m_cNone : m_cBack);
    ui->ixregView->setText(tmp);

    tmp = int2hex(cpu.registers.IY, 6);
    ui->iyregView->setPalette(tmp == ui->iyregView->text() ? m_cNone : m_cBack);
    ui->iyregView->setText(tmp);

    tmp = int2hex(cpu.registers._AF, 4);
    ui->af_regView->setPalette(tmp == ui->af_regView->text() ? m_cNone : m_cBack);
    ui->af_regView->setText(tmp);

    tmp = int2hex(cpu.registers._HL, 6);
    ui->hl_regView->setPalette(tmp == ui->hl_regView->text() ? m_cNone : m_cBack);
    ui->hl_regView->setText(tmp);

    tmp = int2hex(cpu.registers._DE, 6);
    ui->de_regView->setPalette(tmp == ui->de_regView->text() ? m_cNone : m_cBack);
    ui->de_regView->setText(tmp);

    tmp = int2hex(cpu.registers._BC, 6);
    ui->bc_regView->setPalette(tmp == ui->bc_regView->text() ? m_cNone : m_cBack);
    ui->bc_regView->setText(tmp);

    tmp = int2hex(cpu.registers.SPS, 4);
    ui->spsregView->setPalette(tmp == ui->spsregView->text() ? m_cNone : m_cBack);
    ui->spsregView->setText(tmp);

    tmp = int2hex(cpu.registers.SPL, 6);
    ui->splregView->setPalette(tmp == ui->splregView->text() ? m_cNone : m_cBack);
    ui->splregView->setText(tmp);

    tmp = int2hex(cpu.registers.MBASE, 2);
    ui->mbregView->setPalette(tmp == ui->mbregView->text() ? m_cNone : m_cBack);
    ui->mbregView->setText(tmp);

    tmp = int2hex(cpu.registers.I, 4);
    ui->iregView->setPalette(tmp == ui->iregView->text() ? m_cNone : m_cBack);
    ui->iregView->setText(tmp);

    tmp = int2hex(cpu.IM - !!cpu.IM, 1);
    ui->imregView->setPalette(tmp == ui->imregView->text() ? m_cNone : m_cBack);
    ui->imregView->setText(tmp);

    tmp = int2hex(cpu.registers.PC, 6);
    ui->pcregView->setPalette(tmp == ui->pcregView->text() ? m_cNone : m_cBack);
    ui->pcregView->setText(tmp);

    tmp = int2hex(cpu.registers.R >> 1 | cpu.registers.R << 7, 2);
    ui->rregView->setPalette(tmp == ui->rregView->text() ? m_cNone : m_cBack);
    ui->rregView->setText(tmp);

    tmp = int2hex(lcd.upbase, 6);
    ui->lcdbaseView->setPalette(tmp == ui->lcdbaseView->text() ? m_cNone : m_cBack);
    ui->lcdbaseView->setText(tmp);

    tmp = int2hex(lcd.upcurr, 6);
    ui->lcdcurrView->setPalette(tmp == ui->lcdcurrView->text() ? m_cNone : m_cBack);
    ui->lcdcurrView->setText(tmp);

    tmp = QString::number(sched.clockRates[CLOCK_CPU]);
    ui->freqView->setPalette(tmp == ui->freqView->text() ? m_cNone : m_cBack);
    ui->freqView->setText(tmp);

    tmp = QString::number(debug.totalCycles - (m_ignoreDmaCycles ? debug.dmaCycles : 0));
    ui->cycleView->setPalette(tmp == ui->cycleView->text() ? m_cNone : m_cBack);
    ui->cycleView->setText(tmp);

    tmp = QString::number(rtc.readSec);
    ui->seconds->setPalette(tmp == ui->seconds->text() ? m_cNone : m_cBack);
    ui->seconds->setText(tmp);

    tmp = QString::number(rtc.readMin);
    ui->minutes->setPalette(tmp == ui->minutes->text() ? m_cNone : m_cBack);
    ui->minutes->setText(tmp);

    tmp = QString::number(rtc.readHour);
    ui->hours->setPalette(tmp == ui->hours->text() ? m_cNone : m_cBack);
    ui->hours->setText(tmp);

    tmp = QString::number(rtc.readDay);
    ui->days->setPalette(tmp == ui->days->text() ? m_cNone : m_cBack);
    ui->days->setText(tmp);

    tmp = QString::number(gpt.timer[0].counter);
    ui->timer1->setPalette(tmp == ui->timer1->text() ? m_cNone : m_cBack);
    ui->timer1->setText(tmp);

    tmp = QString::number(gpt.timer[0].reset);
    ui->timer1r->setPalette(tmp == ui->timer1r->text() ? m_cNone : m_cBack);
    ui->timer1r->setText(tmp);

    tmp = QString::number(gpt.timer[1].counter);
    ui->timer2->setPalette(tmp == ui->timer2->text() ? m_cNone : m_cBack);
    ui->timer2->setText(tmp);

    tmp = QString::number(gpt.timer[1].reset);
    ui->timer2r->setPalette(tmp == ui->timer2r->text() ? m_cNone : m_cBack);
    ui->timer2r->setText(tmp);

    tmp = QString::number(gpt.timer[2].counter);
    ui->timer3->setPalette(tmp == ui->timer3->text() ? m_cNone : m_cBack);
    ui->timer3->setText(tmp);

    tmp = QString::number(gpt.timer[2].reset);
    ui->timer3r->setPalette(tmp == ui->timer3r->text() ? m_cNone : m_cBack);
    ui->timer3r->setText(tmp);

    batterySetCharging(control.batteryCharging);
    batterySet(control.setBatteryStatus);

    ui->bppView->setCurrentIndex((lcd.control >> 1) & 7);

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkADL->blockSignals(true);
    ui->checkADL->setChecked(cpu.ADL);
    ui->checkADL->blockSignals(false);
    ui->checkMADL->setChecked(cpu.MADL);
    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);

    ui->checkPowered->setChecked(lcd.control & 0x800);
    ui->checkBEPO->setChecked(lcd.control & 0x400);
    ui->checkBEBO->setChecked(lcd.control & 0x200);
    ui->checkBGR->setChecked(lcd.control & 0x100);
    ui->brightnessSlider->setValue(backlight.brightness);

    m_ports->blockSignals(true);
    m_watchpoints->blockSignals(true);

    for (int i = 0; i < m_ports->rowCount(); i++) {
        portPopulate(i);
    }

    m_ports->blockSignals(false);
    m_watchpoints->blockSignals(false);

    osUpdate();
    stackUpdate();
    disasmUpdateAddr(m_prevDisasmAddr = cpu.registers.PC, true);

    memUpdate();
}

// ------------------------------------------------
// Clock items
// ------------------------------------------------

void MainWindow::debugZeroCycles() {
    debug.totalCycles = 0;
    debug.dmaCycles = 0;
    ui->cycleView->setText(QStringLiteral("0"));
    ui->cycleView->repaint();
}

// ------------------------------------------------
// Breakpoints
// ------------------------------------------------

void MainWindow::breakSetPrev(QTableWidgetItem *item) {
    if (item->text().isEmpty()) {
        return;
    }
    m_prevBreakAddr = m_breakpoints->item(item->row(), BREAK_ADDR_LOC)->text();
}

void MainWindow::breakRemoveRow(int row) {
    uint32_t address = static_cast<uint32_t>(hex2int(m_breakpoints->item(row, BREAK_ADDR_LOC)->text()));

    debug_watch(address, DBG_MASK_EXEC, false);
    if (!m_guiAdd && !m_useSoftCom) {
        disasmUpdate();
    }
    m_breakpoints->removeRow(row);
}

void MainWindow::breakRemoveSelected() {
    for (int row = 0; row < m_breakpoints->rowCount(); row++){
        if (sender() == m_breakpoints->cellWidget(row, BREAK_REMOVE_LOC)) {
            breakRemoveRow(row);
            break;
        }
    }
}

void MainWindow::breakRemove(uint32_t address) {
    for (int row = 0; row < m_breakpoints->rowCount(); row++) {
        uint32_t test = static_cast<uint32_t>(hex2int(m_breakpoints->item(row, BREAK_ADDR_LOC)->text()));
        if (address == test) {
            breakRemoveRow(row);
            break;
        }
    }
}

int MainWindow::breakGetMask(int row) {
    int mask;
    if (static_cast<QAbstractButton *>(m_breakpoints->cellWidget(row, BREAK_ENABLE_LOC))->isChecked()) {
        mask = DBG_MASK_EXEC;
    } else {
        mask = DBG_MASK_NONE;
    }
    return mask;
}

void MainWindow::breakModified(QTableWidgetItem *item) {
    int row = item->row();
    int col = item->column();
    QString addrStr;
    uint32_t addr;

    if (col == BREAK_ENABLE_LOC) {
        addr = static_cast<uint32_t>(hex2int(m_breakpoints->item(row, BREAK_ADDR_LOC)->text()));
        debug_watch(addr, DBG_MASK_EXEC, item->checkState() == Qt::Checked);
    } else if (col == BREAK_NAME_LOC) {
        updateLabels();
    } else if (col == BREAK_ADDR_LOC){
        std::string s = item->text().toUpper().toStdString();
        QString equate;
        int mask;

        equate = getAddressOfEquate(s);
        if (!equate.isEmpty()) {
            s = equate.toStdString();
            m_breakpoints->blockSignals(true);
            if (m_breakpoints->item(row, BREAK_NAME_LOC)->text() == (QStringLiteral("Label") + QString::number(row))) {
                m_breakpoints->item(row, BREAK_NAME_LOC)->setText(item->text());
            }
            m_breakpoints->blockSignals(false);
        }

        if (isNotValidHex(s) || s.length() > 6) {
            item->setText(m_prevBreakAddr);
            return;
        }

        addr = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));
        addrStr = int2hex(addr, 6);

        m_breakpoints->blockSignals(true);

        // Return if address is already set
        for (int i = 0; i < m_breakpoints->rowCount(); i++) {
            if (m_breakpoints->item(i, BREAK_ADDR_LOC)->text() == addrStr && i != row) {
                item->setText(m_prevBreakAddr);
                m_breakpoints->blockSignals(false);
                return;
            }
        }

        mask = breakGetMask(row);

        if (m_prevBreakAddr != DEBUG_UNSET_ADDR) {
            debug_watch(hex2int(m_prevBreakAddr), DBG_MASK_EXEC, false);
        }
        item->setText(addrStr);
        debug_watch(addr, mask, true);
        m_breakpoints->blockSignals(false);
    }
    disasmUpdate();
}

QString MainWindow::breakNextLabel() {
    return QStringLiteral("Label") + QString::number(m_breakpoints->rowCount());
}

QString MainWindow::watchNextLabel() {
    return QStringLiteral("Label") + QString::number(m_watchpoints->rowCount());
}

void MainWindow::breakAddSlot() {
    breakAdd(breakNextLabel(), 0, true, false, true);
}

void MainWindow::breakAddGui() {
    uint32_t address = static_cast<uint32_t>(hex2int(m_disasm->getSelectedAddr()));

    QTextCursor c = m_disasm->textCursor();
    c.setCharFormat(m_disasm->currentCharFormat());

    m_guiAdd = true;

    breakAdd(breakNextLabel(), address, true, true, false);

    m_guiAdd = false;

    int32_t base = disasm.base;
    int32_t next = disasm.next;

    disasm.base = address;
    disasm.highlight.breakP = false;
    disasmGet();
    disasm.base = base;
    disasm.next = next;

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+9, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the red dot
    if (disasm.highlight.breakP) {
        c.insertHtml(QStringLiteral("<font color='#FFA3A3'>&#9679;</font>"));
    } else {
        c.insertText(QStringLiteral(" "));
    }

    if (m_disasm->labelCheck()) {
        disasmUpdate();
    }
}

bool MainWindow::breakAdd(const QString &label, uint32_t addr, bool enabled, bool toggle, bool unset) {
    const int row = m_breakpoints->rowCount();
    QString addrStr;

    if (unset) {
        addrStr = DEBUG_UNSET_ADDR;
    } else {
        addrStr = int2hex((addr &= 0xFFFFFF), 6).toUpper();
    }

    // return if address is already set
    for (int i = 0; i < row; i++) {
        if (m_breakpoints->item(i, BREAK_ADDR_LOC)->text() == addrStr) {
            if (addrStr != DEBUG_UNSET_ADDR) {
                if (!m_useSoftCom) {
                    m_breakpoints->selectRow(i);
                    if (toggle) {
                        breakRemoveRow(i);
                    }
                } else {
                    ui->lcd->setFocus();
                }
                return false;
            }
        }
    }

    m_breakpoints->blockSignals(true);
    m_breakpoints->setRowCount(row + 1);

    QToolButton *btnRemove = new QToolButton;
    btnRemove->setIcon(m_iconRemove);

    QToolButton *btnEnable = new QToolButton;
    btnEnable->setIcon(enabled ? m_iconCheck : m_iconCheckGray);
    btnEnable->setCheckable(true);
    btnEnable->setChecked(enabled);

    connect(btnRemove, &QToolButton::clicked, this, &MainWindow::breakRemoveSelected);
    connect(btnEnable, &QToolButton::clicked, [this, btnEnable](bool checked) { btnEnable->setIcon(checked ? m_iconCheck : m_iconCheckGray); });

    QTableWidgetItem *itemLabel = new QTableWidgetItem(label);
    QTableWidgetItem *itemAddr = new QTableWidgetItem(addrStr);
    QTableWidgetItem *itemBreak = new QTableWidgetItem;
    QTableWidgetItem *itemRemove = new QTableWidgetItem;

    m_breakpoints->setItem(row, BREAK_NAME_LOC, itemLabel);
    m_breakpoints->setItem(row, BREAK_ADDR_LOC, itemAddr);
    m_breakpoints->setItem(row, BREAK_ENABLE_LOC, itemBreak);
    m_breakpoints->setItem(row, BREAK_REMOVE_LOC, itemRemove);
    m_breakpoints->setCellWidget(row, BREAK_REMOVE_LOC, btnRemove);
    m_breakpoints->setCellWidget(row, BREAK_ENABLE_LOC, btnEnable);

    m_breakpoints->setCurrentCell(row, BREAK_ADDR_LOC);

    if (addrStr != DEBUG_UNSET_ADDR) {
        debug_watch(addr, DBG_MASK_EXEC, enabled);
    }

    if (!m_guiAdd && !m_useSoftCom) {
        disasmUpdate();
    }

    m_prevBreakAddr = addrStr;
    m_breakpoints->blockSignals(false);

    if (m_useSoftCom) {
        ui->lcd->setFocus();
    }

    m_breakpoints->setVisible(false);
    m_breakpoints->resizeColumnsToContents();
    m_breakpoints->setVisible(true);
    return true;
}

// ------------------------------------------------
// Ports
// ------------------------------------------------

void MainWindow::portSetPrev(QTableWidgetItem *item) {
    if (item->text().isEmpty()) {
        return;
    }
    m_prevPortAddr = m_ports->item(item->row(), PORT_ADDR_LOC)->text();
}

void MainWindow::portRemoveRow(int row) {
    uint16_t port = static_cast<uint16_t>(hex2int(m_ports->item(row, PORT_ADDR_LOC)->text()));
    debug_ports(port, ~DBG_MASK_NONE, false);
    m_ports->removeRow(row);
}

void MainWindow::portRemoveSelected() {
    for (int row = 0; row < m_ports->rowCount(); row++){
        if (sender() == m_ports->cellWidget(row, PORT_REMOVE_LOC)) {
            portRemoveRow(row);
            break;
        }
    }
}

void MainWindow::portPopulate(int currRow) {
    uint16_t port = static_cast<uint16_t>(hex2int(m_ports->item(currRow, PORT_ADDR_LOC)->text()));
    uint8_t read = static_cast<uint8_t>(port_peek_byte(port));

    m_ports->item(currRow, PORT_VALUE_LOC)->setText(int2hex(read, 2));
}

void MainWindow::portAddSlot() {
    portAdd(0, DBG_MASK_NONE, true);
}

bool MainWindow::portAdd(uint16_t port, int mask, bool unset) {
    const int row = m_ports->rowCount();
    QString portStr;
    uint8_t data = 0;

    if (unset) {
        portStr = DEBUG_UNSET_PORT;
    } else {
        portStr = int2hex(port, 4).toUpper();
        if (guiDebug) {
            data = port_peek_byte(port);
        }
    }

    // return if port is already set
    for (int i = 0; i < row; i++) {
        if (m_ports->item(i, PORT_ADDR_LOC)->text() == portStr) {
            if (portStr != DEBUG_UNSET_PORT) {
                return false;
            }
        }
    }

    m_ports->setRowCount(row + 1);
    m_ports->blockSignals(true);

    QToolButton *btnRemove = new QToolButton;
    btnRemove->setIcon(m_iconRemove);

    QToolButton *btnRead = new QToolButton;
    btnRead->setIcon((mask & DBG_MASK_PORT_READ) ? m_iconCheck : m_iconCheckGray);
    btnRead->setCheckable(true);
    btnRead->setChecked(mask & DBG_MASK_PORT_READ);

    QToolButton *btnWrite = new QToolButton;
    btnWrite->setIcon((mask & DBG_MASK_PORT_WRITE) ? m_iconCheck : m_iconCheckGray);
    btnWrite->setCheckable(true);
    btnWrite->setChecked(mask & DBG_MASK_PORT_WRITE);

    QToolButton *btnFreeze = new QToolButton;
    btnFreeze->setIcon((mask & DBG_MASK_PORT_FREEZE) ? m_iconCheck : m_iconCheckGray);
    btnFreeze->setCheckable(true);
    btnFreeze->setChecked(mask & DBG_MASK_PORT_FREEZE);

    connect(btnRemove, &QToolButton::clicked, this, &MainWindow::portRemoveSelected);
    connect(btnRead, &QToolButton::clicked, [this, btnRead](bool checked) { btnRead->setIcon(checked ? m_iconCheck : m_iconCheckGray); });
    connect(btnWrite, &QToolButton::clicked, [this, btnWrite](bool checked) { btnWrite->setIcon(checked ? m_iconCheck : m_iconCheckGray); });
    connect(btnFreeze, &QToolButton::clicked, [this, btnFreeze](bool checked) { btnFreeze->setIcon(checked ? m_iconCheck : m_iconCheckGray); });

    QTableWidgetItem *itemAddr = new QTableWidgetItem(portStr);
    QTableWidgetItem *itemData = new QTableWidgetItem(int2hex(data, 2));
    QTableWidgetItem *itemRead = new QTableWidgetItem;
    QTableWidgetItem *itemWrite = new QTableWidgetItem;
    QTableWidgetItem *itemFreeze = new QTableWidgetItem;
    QTableWidgetItem *itemRemove = new QTableWidgetItem;

    m_ports->setItem(row, PORT_ADDR_LOC, itemAddr);
    m_ports->setItem(row, PORT_VALUE_LOC, itemData);
    m_ports->setItem(row, PORT_READ_LOC, itemRead);
    m_ports->setItem(row, PORT_WRITE_LOC, itemWrite);
    m_ports->setItem(row, PORT_FREEZE_LOC, itemFreeze);
    m_ports->setItem(row, PORT_REMOVE_LOC, itemRemove);
    m_ports->setCellWidget(row, PORT_REMOVE_LOC, btnRemove);
    m_ports->setCellWidget(row, PORT_READ_LOC, btnRead);
    m_ports->setCellWidget(row, PORT_WRITE_LOC, btnWrite);
    m_ports->setCellWidget(row, PORT_FREEZE_LOC, btnFreeze);

    m_ports->selectRow(row);
    m_prevPortAddr = portStr;
    m_ports->blockSignals(false);
    m_ports->setVisible(false);
    m_ports->resizeColumnsToContents();
    m_ports->setVisible(true);
    return true;
}

int MainWindow::portGetMask(int row) {
    unsigned int mask = 0;
    if (static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_READ_LOC))->isChecked()) {
        mask |= DBG_MASK_PORT_READ;
    }
    if (static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_WRITE_LOC))->isChecked()) {
        mask |= DBG_MASK_PORT_WRITE;
    }
    if (static_cast<QAbstractButton *>(m_ports->cellWidget(row, PORT_FREEZE_LOC))->isChecked()) {
        mask |= DBG_MASK_PORT_FREEZE;
    }
    return mask;
}

void MainWindow::portModified(QTableWidgetItem *item) {
    int row = item->row();
    int col = item->column();

    if (col == PORT_READ_LOC || col == PORT_WRITE_LOC || col == PORT_FREEZE_LOC) {
        uint16_t port = static_cast<uint16_t>(hex2int(m_ports->item(row, PORT_ADDR_LOC)->text()));
        unsigned int mask = DBG_MASK_NONE;

        if (col == PORT_READ_LOC) {   // Break on read
            mask = DBG_MASK_PORT_READ;
        }
        if (col == PORT_WRITE_LOC) {  // Break on write
            mask = DBG_MASK_PORT_WRITE;
        }
        if (col == PORT_FREEZE_LOC) { // Freeze
            mask = DBG_MASK_PORT_FREEZE;
        }
        debug_ports(port, mask, item->checkState() == Qt::Checked);
    } else if (col == PORT_ADDR_LOC) {
        std::string s = item->text().toUpper().toStdString();
        int mask;

        if (isNotValidHex(s) || s.length() > 4) {
            item->setText(m_prevPortAddr);
            return;
        }

        uint16_t port = static_cast<uint16_t>(hex2int(QString::fromStdString(s)));
        uint8_t data = port_peek_byte(port);
        QString portStr = int2hex(port, 4);

        m_ports->blockSignals(true);

        // return if port is already set
        for (int i=0; i<m_ports->rowCount(); i++) {
            if (m_ports->item(i, PORT_ADDR_LOC)->text() == portStr && i != row) {
                item->setText(m_prevPortAddr);
                m_ports->blockSignals(false);
                return;
            }
        }

        if (m_prevPortAddr != DEBUG_UNSET_PORT) {
            debug_ports(hex2int(m_prevPortAddr), ~DBG_MASK_NONE, false);
        }

        mask = portGetMask(row);
        debug_ports(port, mask, true);
        item->setText(portStr);
        m_ports->item(row, PORT_VALUE_LOC)->setText(int2hex(data, 2));
    } else if (col == PORT_VALUE_LOC) {
        if (m_ports->item(row, PORT_ADDR_LOC)->text() != DEBUG_UNSET_PORT) {
            uint8_t pdata = static_cast<uint8_t>(hex2int(item->text()));
            uint16_t port = static_cast<uint16_t>(hex2int(m_ports->item(row, PORT_ADDR_LOC)->text()));

            port_poke_byte(port, pdata);
            item->setText(int2hex(port_peek_byte(port), 2));
        }
    }
    m_ports->blockSignals(false);
}

// ------------------------------------------------
// Watchpoints
// ------------------------------------------------

void MainWindow::watchSetPrev(QTableWidgetItem *item) {
    if (item->text().isEmpty()) {
        return;
    }

    if (item == m_watchpoints->item(item->row(), WATCH_LOW_LOC)) {
        m_prevWatchLow = m_watchpoints->item(item->row(), WATCH_LOW_LOC)->text();
    }
    if (item == m_watchpoints->item(item->row(), WATCH_HIGH_LOC)) {
        m_prevWatchHigh = m_watchpoints->item(item->row(), WATCH_HIGH_LOC)->text();
    }
}

void MainWindow::watchRemoveRow(int row) {
    if (m_watchpoints->item(row, WATCH_LOW_LOC)->text() != DEBUG_UNSET_ADDR &&
        m_watchpoints->item(row, WATCH_HIGH_LOC)->text() != DEBUG_UNSET_ADDR) {
        uint32_t low = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
        uint32_t high = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_HIGH_LOC)->text()));

        for (uint32_t addr = low; addr <= high; addr++) {
            debug_watch(addr, DBG_MASK_READ | DBG_MASK_WRITE, false);
        }

        if (!m_guiAdd && !m_useSoftCom) {
            disasmUpdate();
        }
    }
    m_watchpoints->removeRow(row);
    watchUpdate();
}

void MainWindow::watchRemoveSelected() {
    for (int row = 0; row < m_watchpoints->rowCount(); row++){
        if (sender() == m_watchpoints->cellWidget(row, WATCH_REMOVE_LOC)) {
            watchRemoveRow(row);
            break;
        }
    }
}

void MainWindow::watchRemove(uint32_t address) {
    for (int row = 0; row < m_watchpoints->rowCount(); row++) {
        uint32_t test = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
        if (address == test) {
            watchRemoveRow(row);
            break;
        }
    }
}

void MainWindow::watchAddGuiR() {
    m_watchGUIMask = DBG_MASK_READ;
    watchAddGui();
}

void MainWindow::watchAddGuiW() {
    m_watchGUIMask = DBG_MASK_WRITE;
    watchAddGui();
}

void MainWindow::watchAddGuiRW() {
    m_watchGUIMask = DBG_MASK_READ | DBG_MASK_WRITE;
    watchAddGui();
}

void MainWindow::watchAddGui() {
    unsigned int mask = m_watchGUIMask;
    uint32_t address = static_cast<uint32_t>(hex2int(m_disasm->getSelectedAddr()));

    QTextCursor c = m_disasm->textCursor();
    c.setCharFormat(m_disasm->currentCharFormat());

    m_guiAdd = true;

    watchAdd(watchNextLabel(), address, address, mask, true, false);

    m_guiAdd = false;

    int32_t base = disasm.base;
    int32_t next = disasm.next;

    disasm.base = address;
    disasm.highlight.watchR = false;
    disasm.highlight.watchW = false;
    disasmGet();

    disasm.base = base;
    disasm.next = next;

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+7, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the green dot
    if (disasm.highlight.watchR) {
        c.insertHtml(QStringLiteral("<font color='#A3FFA3'>&#9679;</font>"));
    } else {
        c.insertText(QStringLiteral(" "));
    }

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+8, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the blue dot
    if (disasm.highlight.watchW) {
        c.insertHtml(QStringLiteral("<font color='#A3A3FF'>&#9679;</font>"));
    } else {
        c.insertText(QStringLiteral(" "));
    }

    if (m_disasm->labelCheck()) {
        disasmUpdate();
    }
}

void MainWindow::watchAddSlot() {
    watchAdd(watchNextLabel(), 0, 0, DBG_MASK_READ | DBG_MASK_WRITE, false, true);
}

void MainWindow::watchUpdate() {

    // this is needed in the case of overlapping address spaces
    for (int row = 0; row < m_watchpoints->rowCount(); row++) {
        if (m_watchpoints->item(row, WATCH_LOW_LOC)->text() != DEBUG_UNSET_ADDR &&
            m_watchpoints->item(row, WATCH_HIGH_LOC)->text() != DEBUG_UNSET_ADDR) {
            uint32_t low = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
            uint32_t high = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_HIGH_LOC)->text()));
            int mask = watchGetMask(row);

            for (uint32_t addr = low; addr <= high; addr++) {
                debug_watch(addr, mask, true);
            }
        }
    }

    if (!m_guiAdd && !m_useSoftCom) {
        disasmUpdate();
        memUpdate();
    }
}

void MainWindow::watchUpdateRow(int row) {

    // this is needed in the case of overlapping address spaces
    if (m_watchpoints->item(row, WATCH_LOW_LOC)->text() != DEBUG_UNSET_ADDR &&
        m_watchpoints->item(row, WATCH_HIGH_LOC)->text() != DEBUG_UNSET_ADDR) {
        uint32_t low = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
        uint32_t high = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_HIGH_LOC)->text()));

        for (uint32_t addr = low; addr <= high; addr++) {
            debug_watch(addr, DBG_MASK_READ | DBG_MASK_WRITE, false);
        }
    }

    watchUpdate();

    if (!m_guiAdd && !m_useSoftCom) {
        disasmUpdate();
        memUpdate();
    }
}

bool MainWindow::watchAdd(const QString& label, uint32_t low, uint32_t high, int mask, bool toggle, bool unset) {
    const int row = m_watchpoints->rowCount();
    QString lowStr;
    QString highStr;
    QString watchLen;

    if (unset) {
        lowStr = DEBUG_UNSET_ADDR;
        highStr = DEBUG_UNSET_ADDR;
    } else {
        lowStr = int2hex((low &= 0xFFFFFF), 6).toUpper();
        highStr = int2hex((high &= 0xFFFFFF), 6).toUpper();
    }

    // return if address is already set
    for (int i = 0; i < row; i++) {
        if (m_watchpoints->item(i, WATCH_LOW_LOC)->text() == lowStr &&
            m_watchpoints->item(i, WATCH_HIGH_LOC)->text() == highStr) {
            if (lowStr != DEBUG_UNSET_ADDR && highStr != DEBUG_UNSET_ADDR) {
                if (!m_useSoftCom) {
                    m_watchpoints->selectRow(i);
                    if (toggle) {
                        watchRemoveRow(i);
                    }
                } else {
                    ui->lcd->setFocus();
                }
                return false;
            }
        }
    }

    m_watchpoints->blockSignals(true);

    QToolButton *button = new QToolButton();
    button->setIcon(m_iconRemove);
    connect(button, &QToolButton::clicked, this, &MainWindow::watchRemoveSelected);

    QTableWidgetItem *itemLabel = new QTableWidgetItem(label);
    QTableWidgetItem *itemLow = new QTableWidgetItem(lowStr);
    QTableWidgetItem *itemHigh = new QTableWidgetItem(highStr);
    QTableWidgetItem *itemRead = new QTableWidgetItem;
    QTableWidgetItem *itemWrite = new QTableWidgetItem;
    QTableWidgetItem *itemRemove = new QTableWidgetItem;

    QToolButton *btnRead = new QToolButton;
    btnRead->setIcon((mask & DBG_MASK_READ) ? m_iconCheck : m_iconCheckGray);
    btnRead->setCheckable(true);
    btnRead->setChecked((mask & DBG_MASK_READ) ? true : false);

    QToolButton *btnWrite = new QToolButton;
    btnWrite->setIcon((mask & DBG_MASK_WRITE) ? m_iconCheck : m_iconCheckGray);
    btnWrite->setCheckable(true);
    btnWrite->setChecked((mask & DBG_MASK_WRITE) ? true : false);

    connect(btnRead, &QToolButton::clicked, [this, btnRead, row](bool checked) {
        btnRead->setIcon(checked ? m_iconCheck : m_iconCheckGray);
        watchUpdateRow(row);
    });
    connect(btnWrite, &QToolButton::clicked, [this, btnWrite, row](bool checked) {
        btnWrite->setIcon(checked ? m_iconCheck : m_iconCheckGray);
        watchUpdateRow(row);
    });

    m_watchpoints->setRowCount(row + 1);
    m_watchpoints->setItem(row, WATCH_NAME_LOC, itemLabel);
    m_watchpoints->setItem(row, WATCH_LOW_LOC, itemLow);
    m_watchpoints->setItem(row, WATCH_HIGH_LOC, itemHigh);
    m_watchpoints->setItem(row, WATCH_READ_LOC, itemRead);
    m_watchpoints->setItem(row, WATCH_WRITE_LOC, itemWrite);
    m_watchpoints->setItem(row, WATCH_REMOVE_LOC, itemRemove);
    m_watchpoints->setCellWidget(row, WATCH_REMOVE_LOC, button);
    m_watchpoints->setCellWidget(row, WATCH_READ_LOC, btnRead);
    m_watchpoints->setCellWidget(row, WATCH_WRITE_LOC, btnWrite);

    m_watchpoints->setCurrentCell(row, WATCH_LOW_LOC);

    if (!m_guiAdd && !m_useSoftCom) {
        disasmUpdate();
        memUpdate();
    }

    m_prevWatchLow = lowStr;
    m_prevWatchHigh = highStr;
    m_watchpoints->blockSignals(false);

    watchUpdate();

    if (m_useSoftCom) {
        ui->lcd->setFocus();
    }

    m_watchpoints->setVisible(false);
    m_watchpoints->resizeColumnsToContents();
    m_watchpoints->setVisible(true);
    return true;
}

void MainWindow::memUpdate() {
    ramUpdate();
    flashUpdate();
    memDocksUpdate();
}

int MainWindow::watchGetMask(int row) {
    int mask = 0;
    if (static_cast<QAbstractButton *>(m_watchpoints->cellWidget(row, WATCH_READ_LOC))->isChecked()) {
        mask |= DBG_MASK_READ;
    } else {
        mask |= DBG_MASK_NONE;
    }
    if (static_cast<QAbstractButton *>(m_watchpoints->cellWidget(row, WATCH_WRITE_LOC))->isChecked()) {
        mask |= DBG_MASK_WRITE;
    } else {
        mask |= DBG_MASK_NONE;
    }
    return mask;
}

void MainWindow::watchModified(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();
    QString lowStr;
    QString highStr;
    uint32_t addr;

    m_watchpoints->blockSignals(true);

    if (col == WATCH_NAME_LOC) {
        updateLabels();
    } else if (col == WATCH_READ_LOC || col == WATCH_WRITE_LOC) {
        addr = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
        unsigned int mask = DBG_MASK_NONE;

        if (col == WATCH_READ_LOC) { // Break on read
            mask = DBG_MASK_READ;
        } else
        if (col == WATCH_WRITE_LOC) { // Break on write
            mask = DBG_MASK_WRITE;
        }
        debug_watch(addr, mask, item->checkState() == Qt::Checked);
    } else if (col == WATCH_LOW_LOC) {
        std::string s = item->text().toUpper().toStdString();
        QString equate;

        equate = getAddressOfEquate(s);
        if (!equate.isEmpty()) {
            s = equate.toStdString();
            m_watchpoints->blockSignals(true);
            if (m_watchpoints->item(row, WATCH_NAME_LOC)->text() == (QStringLiteral("Label") + QString::number(row))) {
                m_watchpoints->item(row, WATCH_NAME_LOC)->setText(item->text());
            }
            m_watchpoints->blockSignals(false);
        }

        addr = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));
        highStr = m_watchpoints->item(row, WATCH_HIGH_LOC)->text();

        if (isNotValidHex(s) || s.length() > 6 ||
           (highStr != DEBUG_UNSET_ADDR && addr > hex2int(highStr))) {
            item->setText(m_prevWatchLow);
            m_watchpoints->blockSignals(false);
            return;
        }

        lowStr = int2hex(addr, 6);

        // return if address is already set in this range
        for (int i = 0; i < m_watchpoints->rowCount(); i++) {
            if (m_watchpoints->item(i, WATCH_LOW_LOC)->text() == lowStr &&
                m_watchpoints->item(i, WATCH_HIGH_LOC)->text() == highStr &&
                i != row) {
                item->setText(m_prevWatchLow);
                m_watchpoints->blockSignals(false);
                return;
            }
        }

        if (m_prevWatchLow != DEBUG_UNSET_ADDR) {
            uint32_t low = static_cast<uint32_t>(hex2int(m_prevWatchLow));
            uint32_t high = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_HIGH_LOC)->text()));

            for (uint32_t addr = low; addr <= high; addr++) {
                debug_watch(addr, DBG_MASK_READ | DBG_MASK_WRITE, false);
            }
        }


        if (highStr == DEBUG_UNSET_ADDR) {
            m_watchpoints->item(row, WATCH_HIGH_LOC)->setText(lowStr);
        }
        item->setText(lowStr);
    } else if (col == WATCH_HIGH_LOC) {
        std::string s = item->text().toUpper().toStdString();
        QString equate;

        equate = getAddressOfEquate(s);
        if (!equate.isEmpty()) {
            s = equate.toStdString();
            m_watchpoints->blockSignals(true);
            if (m_watchpoints->item(row, WATCH_NAME_LOC)->text() == (QStringLiteral("Label") + QString::number(row))) {
                m_watchpoints->item(row, WATCH_NAME_LOC)->setText(item->text());
            }
            m_watchpoints->blockSignals(false);
        }

        addr = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));
        lowStr = m_watchpoints->item(row, WATCH_LOW_LOC)->text();

        if (isNotValidHex(s) || s.length() > 6 ||
           (lowStr != DEBUG_UNSET_ADDR && addr < hex2int(lowStr))) {
            item->setText(m_prevWatchLow);
            m_watchpoints->blockSignals(false);
            return;
        }

        highStr = int2hex(addr, 6);

        // return if address is already set in this range
        for (int i = 0; i < m_watchpoints->rowCount(); i++) {
            if (m_watchpoints->item(i, WATCH_HIGH_LOC)->text() == highStr &&
                m_watchpoints->item(i, WATCH_LOW_LOC)->text() == lowStr &&
                i != row) {
                item->setText(m_prevWatchLow);
                m_watchpoints->blockSignals(false);
                return;
            }
        }

        if (m_prevWatchHigh != DEBUG_UNSET_ADDR) {
            uint32_t low = static_cast<uint32_t>(hex2int(m_watchpoints->item(row, WATCH_LOW_LOC)->text()));
            uint32_t high = static_cast<uint32_t>(hex2int(m_prevWatchHigh));

            for (uint32_t addr = low; addr <= high; addr++) {
                debug_watch(addr, DBG_MASK_READ | DBG_MASK_WRITE, false);
            }
        }

        if (lowStr == DEBUG_UNSET_ADDR) {
            m_watchpoints->item(row, WATCH_LOW_LOC)->setText(highStr);
        }
        item->setText(highStr);
    }

    m_watchpoints->blockSignals(false);
    disasmUpdate();
    watchUpdate();
}

// ------------------------------------------------
// Battery Status
// ------------------------------------------------

void MainWindow::batterySetCharging(bool checked) {
    control.batteryCharging = checked;
}

void MainWindow::batterySet(int value) {
    control.setBatteryStatus = static_cast<uint8_t>(value);
    ui->sliderBattery->setValue(value);
    ui->labelBattery->setText(QString::number(value * 20) + "%");
}

// ------------------------------------------------
// Disassembly View
// ------------------------------------------------

void MainWindow::disasmScroll(int value) {
    QScrollBar *v = m_disasm->verticalScrollBar();
    if (value >= v->maximum()) {
        v->blockSignals(true);
        disasmLine();
        v->setValue(m_disasm->verticalScrollBar()->maximum() - 1);
        v->blockSignals(false);
    }
}

void MainWindow::stackScroll(int value) {
    QScrollBar *v = ui->stackView->verticalScrollBar();
    if (value >= v->maximum()) {
        v->blockSignals(true);
        stackLine();
        v->setValue(ui->stackView->verticalScrollBar()->maximum() - 1);
        v->blockSignals(false);
    }
}

void MainWindow::equatesClear() {
    m_equateFiles.clear();
    disasm.map.clear();
    disasm.reverse.clear();
    disasmUpdate();
}

void MainWindow::updateLabels() {
    for (int row = 0; row < m_watchpoints->rowCount(); row++) {
        QString next = getAddressOfEquate(m_watchpoints->item(row, WATCH_NAME_LOC)->text().toUpper().toStdString());
        QString old = m_watchpoints->item(row, WATCH_LOW_LOC)->text();
        if (!next.isEmpty() && next != old) {
            unsigned int mask = (m_watchpoints->item(row, WATCH_READ_LOC)->checkState() == Qt::Checked ? DBG_MASK_READ : DBG_MASK_NONE) |
                                (m_watchpoints->item(row, WATCH_WRITE_LOC)->checkState() == Qt::Checked ? DBG_MASK_WRITE : DBG_MASK_NONE);
            // remove old watchpoint and add new one
            m_watchpoints->blockSignals(true);
            debug_watch(static_cast<uint32_t>(hex2int(old)), mask, false);
            m_watchpoints->item(row, WATCH_LOW_LOC)->setText(next);
            debug_watch(static_cast<uint32_t>(hex2int(next)), mask, true);
            m_watchpoints->blockSignals(true);
        }
    }
    for (int row = 0; row < m_breakpoints->rowCount(); row++) {
        QString next = getAddressOfEquate(m_breakpoints->item(row, BREAK_NAME_LOC)->text().toUpper().toStdString());
        QString old = m_breakpoints->item(row, BREAK_ADDR_LOC)->text();
        if (!next.isEmpty() && next != old) {
            int mask = breakGetMask(row);
            m_breakpoints->blockSignals(true);
            debug_watch(static_cast<uint32_t>(hex2int(old)), mask, false);
            m_breakpoints->item(row, BREAK_ADDR_LOC)->setText(next);
            debug_watch(static_cast<uint32_t>(hex2int(next)), mask, true);
            m_breakpoints->blockSignals(false);
        }
    }
}

void MainWindow::equatesRefresh() {
    disasm.map.clear();
    disasm.reverse.clear();
    for (QString &file : m_equateFiles) {
        equatesAddFile(file);
    }
    updateLabels();
    disasmUpdate();
}

void MainWindow::equatesAddDialog() {
    QFileDialog dialog(this);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(m_dir);

    QStringList extFilters;
    extFilters << tr("Equate files (*.inc *.lab *.map)")
               << tr("All Files (*.*)");
    dialog.setNameFilters(extFilters);

    if (dialog.exec()) {
        m_equateFiles.append(dialog.selectedFiles());
        equatesRefresh();
    }
    m_dir = dialog.directory();
}

void MainWindow::equatesAddFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_equateFiles.removeAll(fileName);
        console(QStringLiteral("[CEmu] Debugger couldn't open this equate file (removed): ") + fileName + "\n");
        return;
    }

    QTextStream in(&file);
    QString line;
    if (in.readLineInto(&line) && line.startsWith(QStringLiteral("Segment"))) {
        while ((in.readLineInto(&line) && !line.startsWith(QStringLiteral("Label"))));
        if (!in.readLineInto(&line)) {
            return;
        }
        while (in.readLineInto(&line) && !line.isEmpty()) {
            QStringList split = line.split('=', QString::SkipEmptyParts);
            equatesAddEquate(split.at(0).simplified(), hex2int(split.at(1).simplified()));
        }
    } else {
        QRegularExpression equatesRegexp(QStringLiteral("^\\h*\\??\\h*([.A-Z_a-z][.\\w]*)\\h*(?::?=|\\h\\.?equ(?!\\d))\\h*([%@$]\\S+|\\d\\S*[boh]?)\\h*(?:;.*)?$"),
                                         QRegularExpression::CaseInsensitiveOption);
        do {
            QRegularExpressionMatch matches = equatesRegexp.match(line);
            if (matches.hasMatch()) {
                QString addrStr = matches.captured(2);
                int base = 10;
                if (addrStr.startsWith('%')) {
                    addrStr.remove(0, 1);
                    base = 2;
                } else if (addrStr.startsWith('@')) {
                    addrStr.remove(0, 1);
                    base = 8;
                } else if (addrStr.startsWith('$')) {
                    addrStr.remove(0, 1);
                    base = 16;
                } else if (addrStr.endsWith('b', Qt::CaseInsensitive)) {
                    addrStr.chop(1);
                    base = 2;
                } else if (addrStr.endsWith('o', Qt::CaseInsensitive)) {
                    addrStr.chop(1);
                    base = 8;
                } else if (addrStr.endsWith('h', Qt::CaseInsensitive)) {
                    addrStr.chop(1);
                    base = 16;
                }
                equatesAddEquate(matches.captured(1), addrStr.toUInt(Q_NULLPTR, base));
            }
        } while (in.readLineInto(&line));
    }

    console(QStringLiteral("[CEmu] Loaded equate file: ") + fileName + QStringLiteral("\n"));

    disasmUpdate();
    updateLabels();
}

void MainWindow::equatesAddEquate(const QString &name, uint32_t address) {
    if (address < 0x80) {
        return;
    }
    map_value_t::const_iterator item = disasm.reverse.find(name.toUpper().toStdString());
    if (item != disasm.reverse.end()) {
        if (address == item->second) {
            return;
        } else {
            disasm.reverse.erase(item);
        }
    }
    uint32_t &itemReverse = disasm.reverse[name.toUpper().toStdString()];
    itemReverse = address;
    disasm.map.emplace(address, name.toStdString());
    uint8_t *ptr = static_cast<uint8_t *>(phys_mem_ptr(address - 4, 9));
    if (ptr && ptr[4] == 0xC3 && (ptr[0] == 0xC3 || ptr[8] == 0xC3)) { // jump table?
        uint32_t address2  = ptr[5] | ptr[6] << 8 | ptr[7] << 16;
        if (phys_mem_ptr(address2, 1)) {
            disasm.map.emplace(address2, "_" + name.toStdString());
            uint32_t &itemReverse2 = disasm.reverse["_" + name.toUpper().toStdString()];
            itemReverse2 = address2;
        }
    }
}

void MainWindow::disasmUpdate() {
    disasmUpdateAddr(m_disasm->getSelectedAddr().toInt(Q_NULLPTR, 16), true);
}

void MainWindow::disasmUpdateAddr(int base, bool pane) {
    if (!guiDebug) {
        return;
    }
    m_disasmAddr = base;
    m_disasmPane = pane;
    m_disasmOffsetSet = false;
    disasm.ctx.zdis_adl = adlState(ui->checkADLDisasm->checkState());
    disasm.base = -1;
    disasm.next = m_disasmAddr - ((pane) ? 0x40 : 0);
    if (disasm.next < 0) { disasm.next = 0; }
    int32_t lastAddr = disasm.next + 0x120;
    if (lastAddr > 0xFFFFFF) { lastAddr = 0xFFFFFF; }

    disconnect(m_disasm->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::disasmScroll);
    m_disasm->clear();
    m_disasm->cursorState(false);
    m_disasm->clearAllHighlights();

    while (disasm.next < lastAddr) {
        disasmLine();
    }

    m_disasm->setTextCursor(m_disasmOffset);
    m_disasm->cursorState(true);
    m_disasm->updateAllHighlights();
    m_disasm->centerCursor();
    connect(m_disasm->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::disasmScroll);
}

// ------------------------------------------------
// Misc
// ------------------------------------------------

void MainWindow::gotoPressed() {
    bool accept;

    if (m_gotoAddr.isEmpty()) {
        m_gotoAddr = m_disasm->getSelectedAddr();
    }

    QString address = getAddressString(m_gotoAddr, &accept);

    if (accept) {
        disasmUpdateAddr(hex2int(m_gotoAddr = address), false);
    }
}

void MainWindow::gotoDisasmAddr(uint32_t address) {
    disasmUpdateAddr(address, false);
}

void MainWindow::gotoMemAddr(uint32_t address) {
    if (m_memWidget != Q_NULLPTR) {
        memGoto(m_memWidget, address);
    }
}

void MainWindow::handleCtrlClickText(QPlainTextEdit *edit) {
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        bool ok = true;

        edit->blockSignals(true);

        QTextCursor cursor = edit->textCursor();
        cursor.select(QTextCursor::WordUnderCursor);
        edit->setTextCursor(cursor);

        QString equ = getAddressOfEquate(edit->textCursor().selectedText().toUpper().toStdString());
        uint32_t address;

        if (!equ.isEmpty()) {
            address = hex2int(equ);
        } else {
            address = edit->textCursor().selectedText().toUInt(&ok, 16);
        }

        if (ok) {
            debugForce();
            if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
                gotoMemAddr(address);
            } else {
                gotoDisasmAddr(address);
            }
        }

        edit->blockSignals(false);
    }
}

void MainWindow::handleCtrlClickLine(QLineEdit *edit) {
    if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        bool ok = true;

        edit->blockSignals(true);

        uint32_t address = edit->text().toUInt(&ok, 16);

        if (ok) {
            if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
                gotoMemAddr(address);
            } else {
                gotoDisasmAddr(address);
            }
        }

        edit->blockSignals(false);
    }
}

// ------------------------------------------------
// Tooltips
// ------------------------------------------------

bool MainWindow::eventFilter(QObject *obj, QEvent *e) {
    if (!guiDebug) {
        return QMainWindow::eventFilter(obj, e);
    }

    if (e->type() == QEvent::MouseButtonPress) {
        QString name = obj->objectName();

        if (name.length() > 3) {
            return QMainWindow::eventFilter(obj, e);
        }

        if (name == QStringLiteral("hl"))  gotoMemAddr(hex2int(ui->hlregView->text()));
        if (name == QStringLiteral("de"))  gotoMemAddr(hex2int(ui->deregView->text()));
        if (name == QStringLiteral("bc"))  gotoMemAddr(hex2int(ui->bcregView->text()));
        if (name == QStringLiteral("ix"))  gotoMemAddr(hex2int(ui->ixregView->text()));
        if (name == QStringLiteral("iy"))  gotoMemAddr(hex2int(ui->iyregView->text()));
        if (name == QStringLiteral("hl_")) gotoMemAddr(hex2int(ui->hl_regView->text()));
        if (name == QStringLiteral("de_")) gotoMemAddr(hex2int(ui->de_regView->text()));
        if (name == QStringLiteral("bc_")) gotoMemAddr(hex2int(ui->bc_regView->text()));
        if (name == QStringLiteral("spl")) gotoMemAddr(hex2int(ui->splregView->text()));
        if (name == QStringLiteral("pc"))  gotoMemAddr(hex2int(ui->pcregView->text()));
    } else if (e->type() == QEvent::MouseMove) {
        QString name = obj->objectName();

        if (name.length() < 4) {
            return QMainWindow::eventFilter(obj, e);
        }

        QLineEdit *widget = static_cast<QLineEdit*>(obj);

        unsigned int num  = widget->text().toUInt(Q_NULLPTR, 16);
        unsigned int num0 = num & 255;
        unsigned int num1 = (num >> 8) & 255;
        unsigned int num2 = (num >> 16) & 255;

        QString t;
        QString val  = QString::number(num);
        QString val0 = QString::number(num0);
        QString val1 = QString::number(num1);
        QString val2 = QString::number(num2);

        if (num  > 0x7FFFFF) {
            val += QStringLiteral("\n\t") + QString::number(static_cast<int32_t>(num | 0xFF000000u));
        }
        if (num0 > 0x7F) {
            val0 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num0));
        }
        if (num1 > 0x7F) {
            val1 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num1));
        }
        if (num2 > 0x7F) {
            val2 += QStringLiteral("\t") + QString::number(static_cast<int8_t>(num2));
        }

        if (name == QStringLiteral("afregView"))
            t = QStringLiteral("a:\t") + val1 +
                QStringLiteral("\nf:\t") + val0;
        if (name == QStringLiteral("hlregView"))
            t = QStringLiteral("hl:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nh:\t") + val1 +
                QStringLiteral("\nl:\t") + val0;
        if (name == QStringLiteral("deregView"))
            t = QStringLiteral("de:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nd:\t") + val1 +
                QStringLiteral("\ne:\t") + val0;
        if (name == QStringLiteral("bcregView"))
            t = QStringLiteral("bc:\t") + val +
                QStringLiteral("\nu:\t") + val2 +
                QStringLiteral("\nb:\t") + val1 +
                QStringLiteral("\nc:\t") + val0;
        if (name == QStringLiteral("ixregView"))
            t = QStringLiteral("ix:\t") + val +
                QStringLiteral("\nixh:\t") + val1 +
                QStringLiteral("\nixl:\t") + val0;
        if (name == QStringLiteral("iyregView"))
            t = QStringLiteral("iy:\t") + val +
                QStringLiteral("\niyh:\t") + val1 +
                QStringLiteral("\niyl:\t") + val0;
        if (name == QStringLiteral("af_regView"))
            t = QStringLiteral("a':\t") + val1 +
                QStringLiteral("\nf':\t") + val0;
        if (name == QStringLiteral("hl_regView"))
            t = QStringLiteral("hl':\t") + val +
            QStringLiteral("\nu':\t") + val2 +
            QStringLiteral("\nh':\t") + val1 +
            QStringLiteral("\nl':\t") + val0;
        if (name == QStringLiteral("de_regView"))
            t = QStringLiteral("de':\t") + val +
                QStringLiteral("\nu':\t") + val2 +
                QStringLiteral("\nd':\t") + val1 +
                QStringLiteral("\ne':\t") + val0;
        if (name == QStringLiteral("bc_regView"))
            t = QStringLiteral("bc':\t") + val +
                QStringLiteral("\nu':\t") + val2 +
                QStringLiteral("\nb':\t") + val1 +
                QStringLiteral("\nc':\t") + val0;
        if (name == QStringLiteral("rregView"))
            t = QStringLiteral("r:\t") + val;

        QToolTip::showText(static_cast<QMouseEvent*>(e)->globalPos(), t, widget, widget->rect());
    }
    return QMainWindow::eventFilter(obj, e);
}

// ------------------------------------------------
// Stack
// ------------------------------------------------

bool MainWindow::adlState(int state) {
    bool adl = ui->checkADL->isChecked();
    if (state == Qt::Checked) {
        adl = true;
    } else if (state == Qt::Unchecked) {
        adl = false;
    }
    return adl;
}

void MainWindow::stackUpdate() {
    disconnect(ui->stackView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::stackScroll);
    ui->stackView->clear();

    m_stackAddr = adlState(ui->checkADLStack->checkState()) ? cpu.registers.SPL : cpu.registers.SPS;

    for (int i = 0; i < 20; i++) {
        stackLine();
    }

    ui->stackView->moveCursor(QTextCursor::Start);
    connect(ui->stackView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::stackScroll);
}

void MainWindow::stackLine() {
    int width = adlState(ui->checkADLStack->checkState()) ? 6 : 4;

    QString line = QString(QStringLiteral("<pre><b><font color='#444'>%1</font></b> %2</pre>"))
                   .arg(int2hex(m_stackAddr, width),
                        int2hex(mem_peek_word(m_stackAddr, width == 6), width));
    m_stackAddr = (m_stackAddr + (width >> 1)) & 0xFFFFFF;

    ui->stackView->appendHtml(line);
}

//------------------------------------------------
// TI-OS View
//------------------------------------------------

void MainWindow::osUpdate() {
    if (!m_normalOs) {
        return;
    }

    calc_var_t var;
    QByteArray array;
    QString data;
    QString dataString;

    int index = 0;
    ui->opView->setRowCount(0);
    ui->vatView->setRowCount(0);
    ui->opStack->setRowCount(0);
    ui->fpStack->setRowCount(0);

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    disconnect(ui->fpStack, &QTableWidget::itemChanged, this, &MainWindow::fpModified);
    disconnect(ui->opView, &QTableWidget::itemChanged, this, &MainWindow::opModified);

    for (uint32_t i = 0xD005F8; i < 0xD005F8+77; i += 11) {
        array.clear();
        dataString.clear();

        for (uint32_t j = i; j < i+11; j++) {
            uint8_t ch = mem_peek_byte(j);
            array.append(ch);
            if ((ch < 0x20) || (ch > 0x7e)) {
                ch = '.';
            }
            dataString += QChar(ch);
        }

        data_t vect(array.constData(), array.constEnd() - 2);

        QTableWidgetItem *opAddr = new QTableWidgetItem(int2hex(i, 6));
        QTableWidgetItem *opNumber = new QTableWidgetItem(QStringLiteral("OP") + QString::number(index+1));
        QTableWidgetItem *opData = new QTableWidgetItem(QString(array.toHex()));
        QTableWidgetItem *opString = new QTableWidgetItem(dataString);
        QTableWidgetItem *opValue;
        try {
            opValue = new QTableWidgetItem(QString::fromStdString(tivars::TH_GenericReal::makeStringFromData(vect)));
        } catch (...) {
            opValue = new QTableWidgetItem(TXT_NAN);
        }
        opNumber->setFlags(opNumber->flags() & ~Qt::ItemIsEditable);
        opAddr->setFlags(opNumber->flags() & ~Qt::ItemIsEditable);

        opAddr->setFont(monospace);
        opNumber->setFont(monospace);
        opData->setFont(monospace);
        opString->setFont(monospace);
        opValue->setFont(monospace);

        ui->opView->setRowCount(index+1);

        ui->opView->setItem(index, OP_ADDRESS, opAddr);
        ui->opView->setItem(index, OP_NUMBER, opNumber);
        ui->opView->setItem(index, OP_DATA, opData);
        ui->opView->setItem(index, OP_STRING, opString);
        ui->opView->setItem(index, OP_VALUE, opValue);

        index++;
    }

    uint32_t fpBase = mem_peek_word(0xD0258A, true);
    uint32_t fpTop = mem_peek_word(0xD0258D, true);

    index = 0;

    if (fpTop >= fpBase && (fpTop >= 0xD00000 && fpTop < 0xD40000) && (fpBase >= 0xD00000 && fpBase < 0xD40000)) {
        for (uint32_t i = fpTop; i > fpBase; i -= 9) {
            array.clear();
            dataString.clear();

            for (uint32_t j = i; j < i+9; j++) {
                uint8_t ch = mem_peek_byte(j);
                array.append(ch);
                if ((ch < 0x20) || (ch > 0x7e)) {
                    ch = '.';
                }
                dataString += QChar(ch);
            }

            data_t vect(array.constData(), array.constEnd());

            QTableWidgetItem *fpAddr = new QTableWidgetItem(int2hex(i, 6));
            QTableWidgetItem *fpData = new QTableWidgetItem(QString(array.toHex()));
            QTableWidgetItem *fpString = new QTableWidgetItem(dataString);
            QTableWidgetItem *fpValue;
            try {
                fpValue = new QTableWidgetItem(QString::fromStdString(tivars::TH_GenericReal::makeStringFromData(vect)));
            } catch(...) {
                fpValue = new QTableWidgetItem(TXT_NAN);
            }
            fpAddr->setFlags(fpAddr->flags() & ~Qt::ItemIsEditable);

            fpAddr->setFont(monospace);
            fpData->setFont(monospace);
            fpString->setFont(monospace);
            fpValue->setFont(monospace);

            ui->fpStack->setRowCount(index+1);

            ui->fpStack->setItem(index, FP_ADDRESS, fpAddr);
            ui->fpStack->setItem(index, FP_DATA, fpData);
            ui->fpStack->setItem(index, FP_STRING, fpString);
            ui->fpStack->setItem(index, FP_VALUE, fpValue);

            index++;
        }
    } else {
        ui->fpStack->setEnabled(false);
    }

    uint32_t opBase = mem_peek_word(0xD02590, true);
    uint32_t opTop = mem_peek_word(0xD02593, true);

    index = 0;

    if (opTop <= opBase && (opTop >= 0xD00000 && opTop < 0xD40000) && (opBase >= 0xD00000 && opBase < 0xD40000)) {
        for (uint32_t i = opTop; i < opBase; i++) {
            data.clear();
            data.append(int2hex(mem_peek_byte(i), 2));

            QTableWidgetItem *opAddr = new QTableWidgetItem(int2hex(i, 6));
            QTableWidgetItem *opData = new QTableWidgetItem(data);
            opAddr->setFlags(opAddr->flags() & ~Qt::ItemIsEditable);
            opData->setFlags(opData->flags() & ~Qt::ItemIsEditable);

            opAddr->setFont(monospace);
            opData->setFont(monospace);

            ui->opStack->setRowCount(index+1);

            ui->opStack->setItem(index, OPS_ADDRESS, opAddr);
            ui->opStack->setItem(index, OPS_DATA, opData);

            index++;
        }
    } else {
        ui->opStack->setEnabled(false);
    }

    index = 0;

    vat_search_init(&var);
    while (vat_search_next(&var)) {
        QTableWidgetItem *varAddr = new QTableWidgetItem(int2hex(var.address, 6));
        QTableWidgetItem *varVatAddr = new QTableWidgetItem(int2hex(var.vat, 6));
        QTableWidgetItem *varSize = new QTableWidgetItem(int2hex(var.size, 4));
        QTableWidgetItem *varName = new QTableWidgetItem(QString(calc_var_name_to_utf8(var.name)));
        QTableWidgetItem *varType = new QTableWidgetItem(QString(calc_var_type_names[var.type]));

        varAddr->setFont(monospace);
        varVatAddr->setFont(monospace);
        varSize->setFont(monospace);
        varName->setFont(monospace);
        varType->setFont(monospace);

        ui->vatView->setRowCount(index+1);

        ui->vatView->setItem(index, VAT_ADDRESS, varAddr);
        ui->vatView->setItem(index, VAT_VAT_ADDRESS, varVatAddr);
        ui->vatView->setItem(index, VAT_SIZE, varSize);
        ui->vatView->setItem(index, VAT_NAME, varName);
        ui->vatView->setItem(index, VAT_TYPE, varType);

        index++;
    }

    ui->vatView->resizeColumnToContents(VAT_ADDRESS);
    ui->vatView->resizeColumnToContents(VAT_VAT_ADDRESS);
    ui->vatView->resizeColumnToContents(VAT_NAME);
    ui->vatView->resizeColumnToContents(VAT_TYPE);
    ui->vatView->resizeColumnToContents(VAT_SIZE);

    connect(ui->opView, &QTableWidget::itemChanged, this, &MainWindow::opModified);
    connect(ui->fpStack, &QTableWidget::itemChanged, this, &MainWindow::fpModified);
}

void MainWindow::opModified(QTableWidgetItem *item) {
    int col = item->column();
    int row = item->row();

    QString txt = item->text();
    QString data;
    QByteArray array;
    uint32_t addr = hex2int(ui->opView->item(row, OP_ADDRESS)->text());
    array.resize(11);

    sender()->blockSignals(true);

    if (col == OP_DATA) {
        array.fill(0);
        for (int i = 0; i < 11 && i < txt.length() / 2; i++) {
            array[i] = hex2int(txt.mid(i * 2, 2));
        }
    } else if (col == OP_STRING) {
        array.fill('.');
        for (int i = 0; i < 11 && i < txt.length(); i++) {
            array[i] = txt[i].toLatin1();
        }
    } else if (col == OP_VALUE) {
        array.fill(0);
        try {
            data_t value = tivars::TH_GenericReal::makeDataFromString(txt.toStdString());
            for (int i = 0; i < 11 && i < static_cast<int>(value.size()); i++) {
                array[i] = value[i];
            }
        } catch(...) {}
    }

    for (int i = 0; i < 11; i++) {
        mem_poke_byte(addr + i, array[i]);
    }

    array.clear();
    data.clear();

    for (uint32_t j = addr; j < addr + 11; j++) {
        uint8_t ch = mem_peek_byte(j);
        array.append(ch);
        if ((ch < 0x20) || (ch > 0x7e)) {
            ch = '.';
        }
        data += QChar(ch);
    }

    data_t vect(array.constData(), array.constEnd() - 2);

    ui->opView->item(row, OP_STRING)->setText(data);
    ui->opView->item(row, OP_DATA)->setText(QString(array.toHex()));
    try {
        ui->opView->item(row, OP_VALUE)->setText(QString::fromStdString(tivars::TH_GenericReal::makeStringFromData(vect)));
    } catch(...) {
        ui->opView->item(row, OP_VALUE)->setText(TXT_NAN);
    }

    sender()->blockSignals(false);
}

void MainWindow::fpModified(QTableWidgetItem *item) {
    int col = item->column();
    int row = item->row();

    QString txt = item->text();
    QString data;
    QByteArray array;
    uint32_t addr = hex2int(ui->fpStack->item(row, FP_ADDRESS)->text());
    array.resize(11);

    sender()->blockSignals(true);

    if (col == FP_DATA) {
        array.fill(0);
        for (int i = 0; i < 9 && i < txt.length() / 2; i++) {
            array[i] = hex2int(txt.mid(i * 2, 2));
        }
    } else if (col == FP_STRING) {
        array.fill('.');
        for (int i = 0; i < 9 && i < txt.length(); i++) {
            array[i] = txt[i].toLatin1();
        }
    } else if (col == FP_VALUE) {
        array.fill(0);
        try {
            data_t value = tivars::TH_GenericReal::makeDataFromString(txt.toStdString());
            for (int i = 0; i < 9 && i < static_cast<int>(value.size()); i++) {
                array[i] = value[i];
            }
        } catch(...) {}
    }

    for (int i = 0; i < 9; i++) {
        mem_poke_byte(addr + i, array[i]);
    }

    array.clear();
    data.clear();

    for (uint32_t j = addr; j < addr + 9; j++) {
        uint8_t ch = mem_peek_byte(j);
        array.append(ch);
        if ((ch < 0x20) || (ch > 0x7e)) {
            ch = '.';
        }
        data += QChar(ch);
    }

    data_t vect(array.constData(), array.constEnd());

    ui->fpStack->item(row, FP_STRING)->setText(data);
    ui->fpStack->item(row, FP_DATA)->setText(QString(array.toHex()));
    try {
        ui->fpStack->item(row, FP_VALUE)->setText(QString::fromStdString(tivars::TH_GenericReal::makeStringFromData(vect)));
    } catch(...) {
        ui->fpStack->item(row, FP_VALUE)->setText(TXT_NAN);
    }

    sender()->blockSignals(false);
}

void MainWindow::contextOp(const QPoint &posa) {
    QTableWidget *obj = static_cast<QTableWidget*>(sender());
    if (!obj->rowCount() || !obj->selectionModel()->isSelected(obj->currentIndex())) {
        return;
    }

    QString gotoMem = tr("Goto Memory View");
    QString copyAddr = tr("Copy Address");
    QString copyData = tr("Copy Data");
    QPoint globalPos = obj->mapToGlobal(posa);

    QString addr = obj->item(obj->selectionModel()->selectedRows().first().row(), OP_ADDRESS)->text();
    QString data = obj->item(obj->selectionModel()->selectedRows().first().row(), obj->objectName() == QStringLiteral("opView") ? 2 : 1)->text();

    QMenu menu;
    menu.addAction(gotoMem);
    menu.addSeparator();
    menu.addAction(copyAddr);
    menu.addAction(copyData);

    QAction *item = menu.exec(globalPos);
    if (item) {
        if (item->text() == gotoMem) {
            gotoMemAddr(hex2int(addr));
        }
        if (item->text() == copyAddr) {
            qApp->clipboard()->setText(addr);
        }
        if (item->text() == copyData) {
            qApp->clipboard()->setText(data);
        }
    }
}

void MainWindow::contextVat(const QPoint &posa) {
    QTableWidget *obj = static_cast<QTableWidget*>(sender());
    if (!obj->rowCount() || !obj->selectionModel()->isSelected(obj->currentIndex())) {
        return;
    }

    QString gotoMem = tr("Goto Memory View");
    QString gotoVat = tr("Goto VAT Memory View");
    QString gotoDisasm = tr("Goto Disasm View");
    QPoint globalPos = obj->mapToGlobal(posa);

    QString addr = obj->item(obj->selectionModel()->selectedRows().first().row(), VAT_ADDRESS)->text();
    QString vatAddr = obj->item(obj->selectionModel()->selectedRows().first().row(), VAT_VAT_ADDRESS)->text();

    QMenu menu;
    menu.addAction(gotoMem);
    menu.addAction(gotoVat);
    menu.addAction(gotoDisasm);

    QAction* item = menu.exec(globalPos);
    if (item) {
        if (item->text() == gotoMem) {
            gotoMemAddr(hex2int(addr));
        }
        if (item->text() == gotoVat) {
            gotoMemAddr(hex2int(vatAddr));
        }
        if (item->text() == gotoDisasm) {
            disasmUpdateAddr(hex2int(addr) + 4, false);
        }
    }
}

void MainWindow::memDocksUpdate() {
    QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    foreach (QDockWidget* dock, docks) {
        if (dock->windowTitle().contains(TXT_MEM_DOCK)) {
            QList<HexWidget*> editChildren = dock->findChildren<HexWidget*>();
            HexWidget *edit = editChildren.first();
            memUpdateEdit(edit);
        }
    }
}

//------------------------------------------------
// Stepping
//------------------------------------------------

void MainWindow::stepIn() {
    if (!guiDebug) {
        return;
    }

    disconnect(m_shortcutStepIn, &QShortcut::activated, this, &MainWindow::stepIn);

    debugSync();
    debugStep(DBG_STEP_IN);
}

void MainWindow::stepOver() {
    if (!guiDebug) {
        return;
    }

    disconnect(m_shortcutStepOver, &QShortcut::activated, this, &MainWindow::stepOver);

    debugSync();
    debugStep(DBG_STEP_OVER);
}

void MainWindow::stepNext() {
    if (!guiDebug) {
        return;
    }

    disconnect(m_shortcutStepNext, &QShortcut::activated, this, &MainWindow::stepNext);

    debugSync();
    debugStep(DBG_STEP_NEXT);
}

void MainWindow::stepOut() {
    if (!guiDebug) {
        return;
    }

    disconnect(m_shortcutStepOut, &QShortcut::activated, this, &MainWindow::stepOut);

    debugSync();
    debugStep(DBG_STEP_OUT);
}

//------------------------------------------------
// Other Functions
//------------------------------------------------

void MainWindow::debugForce() {
    int count = 0;
    if (!guiDebug) {
        debugToggle();
    }
    while (!guiDebug && count < 20) {
        guiDelay(50);
        count++;
    }
}

void MainWindow::addVisualizerDock(const QString &magic, const QString &config) {
    if (m_docksVisualizer.contains(magic)) {
        return;
    }

    m_docksVisualizer.append(magic);
    m_docksVisualizerConfig.append(config);

    DockWidget *dw = new DockWidget(TXT_VISUALIZER_DOCK, this);

    if (m_setup) {
        dw->setFloating(true);
        dw->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, dw->minimumSize(), qApp->desktop()->availableGeometry()));
    }

    VisualizerWidget *widget = new VisualizerWidget(this, config);

    connect(widget, &VisualizerWidget::configChanged, [this, widget, magic]{
        int index;
        if ((index = m_docksVisualizer.indexOf(magic)) != -1) {
            m_docksVisualizerConfig[index] = widget->getConfig();
        }
    });
    connect(dw, &DockWidget::closed, [this, magic]{
        int index;
        if ((index = m_docksVisualizer.indexOf(magic)) != -1) {
            m_docksVisualizer.removeAt(index);
            m_docksVisualizerConfig.removeAt(index);
        }
    });

    dw->setState(m_uiEditMode);
    addDockWidget(Qt::RightDockWidgetArea, dw);
    dw->setObjectName(magic);
    dw->setWidget(widget);

    if (m_setup) {
        dw->show();
        dw->activateWindow();
        dw->raise();
    }
}

void MainWindow::addMemDock(const QString &magic, int bytes, bool ascii) {
    if (m_docksMemory.contains(magic)) {
        return;
    }

    DockWidget *dw;
    dw = new DockWidget(TXT_MEM_DOCK, this);
    dw->setObjectName(magic);
    dw->setState(m_uiEditMode);

    if (m_setup) {
        dw->setFloating(true);
        dw->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, dw->minimumSize(), qApp->desktop()->availableGeometry()));
    }

    m_docksMemory.append(magic);
    m_docksMemoryBytes.append(bytes);
    m_docksMemoryAscii.append(ascii);

    QWidget *widget = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    QPushButton *buttonGoto = new QPushButton(m_iconGoto, tr("Goto"));
    QPushButton *buttonSearch = new QPushButton(m_iconSearch, tr("Search"));
    QToolButton *buttonAscii = new QToolButton();
    QToolButton *buttonSync = new QToolButton();
    buttonAscii->setCheckable(true);
    buttonAscii->setChecked(ascii);
    buttonAscii->setIcon(m_iconAscii);
    buttonSync->setIcon(m_iconSync);
    buttonAscii->setToolTip(tr("Show ASCII"));
    buttonSync->setToolTip(tr("Sync Changes"));
    QSpacerItem *spacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Maximum);
    QSpinBox *spin = new QSpinBox();
    HexWidget *edit = new HexWidget();

    buttonGoto->setEnabled(guiDebug);
    buttonSearch->setEnabled(guiDebug);
    buttonAscii->setEnabled(guiDebug);
    buttonSync->setEnabled(guiDebug);
    spin->setEnabled(guiDebug);
    edit->setEnabled(guiDebug);
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    edit->setAsciiArea(ascii);
    edit->setScrollable(true);

    m_memWidget = edit;

    connect(edit, &HexWidget::customContextMenuRequested, this, &MainWindow::contextMem);
    connect(buttonSearch, &QPushButton::clicked, [this, edit]{ memSearchEdit(edit); });
    connect(buttonGoto, &QPushButton::clicked, [this, edit]{ memGotoEdit(edit); });
    connect(buttonSync, &QToolButton::clicked, [this, edit]{ memSyncEdit(edit); });
    connect(buttonAscii, &QToolButton::toggled, [this, edit, magic]{
        memAsciiToggle(edit);
        int index;
        if ((index = m_docksMemory.indexOf(magic)) != -1) {
            m_docksMemoryAscii[index] = edit->getAsciiArea();
        }
    });
    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, edit, magic](int val){
        edit->setBytesPerLine(val);
        int index;
        if ((index = m_docksMemory.indexOf(magic)) != -1) {
            m_docksMemoryBytes[index] = val;
        }
    });
    connect(edit, &HexWidget::focused, [this, edit]{ m_memWidget = edit; });
    connect(dw, &DockWidget::closed, [this, magic]{
        int index;
        if ((index = m_docksMemory.indexOf(magic)) != -1) {
            m_docksMemory.removeAt(index);
            m_docksMemoryBytes.removeAt(index);
            m_docksMemoryAscii.removeAt(index);
        }
    });

    spin->setMaximum(999);
    spin->setMinimum(1);
    spin->setValue(bytes);

    hlayout->addWidget(buttonGoto);
    hlayout->addWidget(buttonSearch);
    hlayout->addSpacerItem(spacer);
    hlayout->addWidget(buttonAscii);
    hlayout->addWidget(buttonSync);
    hlayout->addWidget(spin);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(edit);
    widget->setLayout(vlayout);
    dw->setWidget(widget);

    if (guiDebug) {
        memUpdateEdit(edit);
    }

    addDockWidget(Qt::RightDockWidgetArea, dw);

    if (m_setup) {
        dw->show();
        dw->activateWindow();
        dw->raise();
    }
}

void MainWindow::setCalcId() {
    bool ok = true;
    const uint8_t *data = mem.flash.block;
    const uint16_t subSize = 5;
    const uint8_t *contents = nullptr;
    uint32_t offset = 0x3B0001;
    uint32_t size;

    /* Outer field. */
    static const uint16_t path[] = { 0x0330, 0x0400 };

    ok = !cert_field_find_path(data + offset, SIZE_FLASH_SECTOR_64K, path, 2, &contents, &size);

    if (!ok) {
        QMessageBox::warning(this, MSG_WARNING, tr("Cannot locate calculator ID in the certificate. This is usually due to an improper ROM dump. Please try another ROM dump using a physical calculator."));
    } else {
        uint32_t field_offset = contents - mem.flash.block;
        uint8_t *ptr = mem.flash.block + field_offset;
        QByteArray array(reinterpret_cast<const char*>(ptr), subSize);
        QString str = QString(array.toHex());

        QString id = QInputDialog::getText(this, tr("CEmu Change Certificate ID"), tr("Old ID: ") + str, QLineEdit::Normal, Q_NULLPTR, &ok);

        if (ok && id.length() == 10) {
            QByteArray ba = QByteArray::fromHex(id.toLatin1());
            memcpy(ptr, ba.data(), subSize);
        }
    }
}

