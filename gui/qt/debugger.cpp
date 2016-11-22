#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QScrollBar>
#include <QtNetwork/QNetworkReply>
#include <fstream>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "debugger.h"
#include "dockwidget.h"
#include "utils.h"

#include "../../core/schedule.h"
#include "../../core/link.h"

// ------------------------------------------------
// Main Debugger things
// ------------------------------------------------

void MainWindow::debuggerGUIDisable(void) {
    debuggerGUISetState(false);
}

void MainWindow::debuggerGUIEnable(void) {
    debuggerGUISetState(true);
}

void MainWindow::debuggerLeave(void) {
    debuggerGUIDisable();
}

void MainWindow::debuggerImport(void) {
    debuggerImportFile(debuggerGetFile(DBG_OPEN));
}

void MainWindow::debuggerExport(void) {
    debuggerExportFile(debuggerGetFile(DBG_SAVE));
}

void MainWindow::debuggerImportFile(QString filename) {
    if (filename.isEmpty()) {
        return;
    }
    int i;

    QSettings debugInfo(filename, QSettings::IniFormat);
    if (debugInfo.value(QStringLiteral("version")) != DBG_VERSION) {
        QMessageBox::critical(this, tr("Invalid Version"), tr("This debugging information is incompatible with this version of CEmu"));
        return;
    }

    // Load the breakpoint information
    QStringList breakpointLabel = debugInfo.value(QStringLiteral("breakpoints/label")).toStringList();
    QStringList breakpointAddress = debugInfo.value(QStringLiteral("breakpoints/address")).toStringList();
    QStringList breakpointEnabled = debugInfo.value(QStringLiteral("breakpoints/enable")).toStringList();
    for (i = 0; i < breakpointLabel.size(); i++) {
        breakpointAdd(breakpointLabel.at(i), hex2int(breakpointAddress.at(i)), breakpointEnabled.at(i) == "y");
    }

    // Load the watchpoint information
    QStringList watchpointLabel = debugInfo.value(QStringLiteral("watchpoints/label")).toStringList();
    QStringList watchpointAddress = debugInfo.value(QStringLiteral("watchpoints/address")).toStringList();
    QStringList watchpointSize = debugInfo.value(QStringLiteral("watchpoints/size")).toStringList();
    QStringList watchpointREnabled = debugInfo.value(QStringLiteral("watchpoints/read")).toStringList();
    QStringList watchpointWEnabled = debugInfo.value(QStringLiteral("watchpoints/write")).toStringList();
    for (i = 0; i < watchpointLabel.size(); i++) {
        unsigned mask = (watchpointREnabled.at(i) == "y" ? DBG_READ_WATCHPOINT : DBG_NO_HANDLE) |
                        (watchpointWEnabled.at(i) == "y" ? DBG_WRITE_WATCHPOINT : DBG_NO_HANDLE);
        watchpointAdd(watchpointLabel.at(i), hex2int(watchpointAddress.at(i)), hex2int(watchpointSize.at(i)), mask);
    }

    // Load the port monitor information
    QStringList portAddress = debugInfo.value(QStringLiteral("portmonitor/address")).toStringList();
    QStringList portREnabled = debugInfo.value(QStringLiteral("portmonitor/read")).toStringList();
    QStringList portWEnabled = debugInfo.value(QStringLiteral("portmonitor/write")).toStringList();
    QStringList portFEnabled = debugInfo.value(QStringLiteral("portmonitor/freeze")).toStringList();
    for (i = 0; i < portAddress.size(); i++) {
        unsigned mask = (portREnabled.at(i) == "y" ? DBG_PORT_READ : DBG_NO_HANDLE)  |
                        (portWEnabled.at(i) == "y" ? DBG_PORT_WRITE : DBG_NO_HANDLE) |
                        (portFEnabled.at(i) == "y" ? DBG_PORT_FREEZE : DBG_NO_HANDLE);
        portAdd(hex2int(portAddress.at(i)), mask);
    }
}

void MainWindow::debuggerExportFile(QString filename) {
    if (filename.isEmpty()) {
        return;
    }
    int i;

    QSettings debugInfo(filename, QSettings::IniFormat);

    // Set the file format version
    debugInfo.setValue(QStringLiteral("version"), DBG_VERSION);

    // Save the breakpoint information
    QStringList breakpointLabel;
    QStringList breakpointAddress;
    QStringList breakpointEnabled;
    for(i = 0; i < ui->breakpointView->rowCount(); i++) {
        breakpointLabel.append(ui->breakpointView->item(i, BREAK_LABEL_LOC)->text());
        breakpointAddress.append(ui->breakpointView->item(i, BREAK_ADDR_LOC)->text());
        breakpointEnabled.append((ui->breakpointView->item(i, BREAK_ENABLE_LOC)->checkState() == Qt::Checked) ? "y" : "n");
    }

    debugInfo.setValue(QStringLiteral("breakpoints/label"), breakpointLabel);
    debugInfo.setValue(QStringLiteral("breakpoints/address"), breakpointAddress);
    debugInfo.setValue(QStringLiteral("breakpoints/enable"), breakpointEnabled);

    // Save watchpoint information
    QStringList watchpointLabel;
    QStringList watchpointAddress;
    QStringList watchpointSize;
    QStringList watchpointREnabled;
    QStringList watchpointWEnabled;
    for(i = 0; i < ui->watchpointView->rowCount(); i++) {
        watchpointLabel.append(ui->watchpointView->item(i, WATCH_LABEL_LOC)->text());
        watchpointAddress.append(ui->watchpointView->item(i, WATCH_ADDR_LOC)->text());
        watchpointSize.append(ui->watchpointView->item(i, WATCH_SIZE_LOC)->text());
        watchpointREnabled.append((ui->watchpointView->item(i, WATCH_READ_LOC)->checkState() == Qt::Checked) ? "y" : "n");
        watchpointWEnabled.append((ui->watchpointView->item(i, WATCH_WRITE_LOC)->checkState() == Qt::Checked) ? "y" : "n");
    }

    debugInfo.setValue(QStringLiteral("watchpoints/label"), watchpointLabel);
    debugInfo.setValue(QStringLiteral("watchpoints/address"), watchpointAddress);
    debugInfo.setValue(QStringLiteral("watchpoints/size"), watchpointSize);
    debugInfo.setValue(QStringLiteral("watchpoints/read"), watchpointREnabled);
    debugInfo.setValue(QStringLiteral("watchpoints/write"), watchpointWEnabled);

    // Save port monitor information
    QStringList portAddress;
    QStringList portREnabled;
    QStringList portWEnabled;
    QStringList portFEnabled;
    for(i = 0; i < ui->portView->rowCount(); i++) {
        portAddress.append(ui->portView->item(i, PORT_ADDR_LOC)->text());
        portREnabled.append((ui->portView->item(i, PORT_READ_LOC)->checkState() == Qt::Checked) ? "y" : "n");
        portWEnabled.append((ui->portView->item(i, PORT_WRITE_LOC)->checkState() == Qt::Checked) ? "y" : "n");
        portFEnabled.append((ui->portView->item(i, PORT_FREEZE_LOC)->checkState() == Qt::Checked) ? "y" : "n");
    }

    debugInfo.setValue(QStringLiteral("portmonitor/address"), portAddress);
    debugInfo.setValue(QStringLiteral("portmonitor/read"), portREnabled);
    debugInfo.setValue(QStringLiteral("portmonitor/write"), portWEnabled);
    debugInfo.setValue(QStringLiteral("portmonitor/freeze"), portFEnabled);

    // Make sure we write the settings
    debugInfo.sync();
}

QString MainWindow::debuggerGetFile(int mode) {
    QString filename;
    QFileDialog dialog(this);

    dialog.setAcceptMode(mode ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(currentDir);
    dialog.setNameFilter(tr("Debugging Info (*.ini)"));
    dialog.setWindowTitle(mode ? tr("Debugger Import") : tr("Debugger Export"));
    dialog.setDefaultSuffix(QStringLiteral("ini"));
    dialog.exec();

    if (!dialog.selectedFiles().isEmpty()) {
        filename = dialog.selectedFiles().first();
    }

    currentDir = dialog.directory();
    return filename;
}

void MainWindow::debuggerRaise(void) {
    debuggerGUIPopulate();
    debuggerGUIEnable();
    connect(stepInShortcut, &QShortcut::activated, this, &MainWindow::stepInPressed);
    connect(stepOverShortcut, &QShortcut::activated, this, &MainWindow::stepOverPressed);
    connect(stepNextShortcut, &QShortcut::activated, this, &MainWindow::stepNextPressed);
    connect(stepOutShortcut, &QShortcut::activated, this, &MainWindow::stepOutPressed);
}

void MainWindow::debuggerExecuteCommand(uint32_t debugAddress, uint8_t command) {

    if (debugAddress == 0xFF) {
        int tmp;
        switch (command) {
            case CMD_ABORT:
                consoleStr(QStringLiteral("[CEmu] Program Aborted.\n"));
                debuggerRaise();
                return; // don't exit the debugger
            case CMD_DEBUG:
                consoleStr(QStringLiteral("[CEmu] Program Entered Debugger.\n"));
                debuggerRaise();
                return; // don't exit the debugger
            case CMD_SET_BREAKPOINT:
                breakpointAdd(breakpointNextLabel(), cpu.registers.DE, true);
                break;
            case CMD_REM_BREAKPOINT:
                breakpointRemoveAddress(cpu.registers.DE);
                break;
            case CMD_SET_R_WATCHPOINT:
                watchpointRemoveAddress(cpu.registers.DE);
                watchpointAdd(watchpointNextLabel(), cpu.registers.DE, cpu.registers.bc.l, DBG_READ_WATCHPOINT);
                break;
            case CMD_SET_W_WATCHPOINT:
                watchpointRemoveAddress(cpu.registers.DE);
                watchpointAdd(watchpointNextLabel(), cpu.registers.DE, cpu.registers.bc.l, DBG_WRITE_WATCHPOINT);
                break;
            case CMD_SET_RW_WATCHPOINT:
                watchpointRemoveAddress(cpu.registers.DE);
                watchpointAdd(watchpointNextLabel(), cpu.registers.DE, cpu.registers.bc.l, DBG_RW_WATCHPOINT);
                break;
            case CMD_REM_WATCHPOINT:
                watchpointRemoveAddress(cpu.registers.DE);
                break;
            case CMD_REM_ALL_BREAK:
                tmp = ui->breakpointView->rowCount();
                for (int i = 0; i < tmp; i++) {
                    ui->breakpointView->selectRow(0);
                    breakpointRemoveSelectedRow();
                }
                break;
            case CMD_REM_ALL_WATCH:
                tmp = ui->watchpointView->rowCount();
                for (int i = 0; i < tmp; i++) {
                    ui->watchpointView->selectRow(0);
                    watchpointRemoveSelectedRow();
                }
                break;
            case CMD_SET_E_WATCHPOINT:
                watchpointRemoveAddress(cpu.registers.DE);
                watchpointAdd(watchpointNextLabel(), cpu.registers.DE, cpu.registers.bc.l, DBG_NO_HANDLE);
                break;
            default:
                consoleErrStr(QStringLiteral("[CEmu] Unknown debug Command: 0x") +
                              QString::number(command, 16).rightJustified(2, '0') +
                              QStringLiteral(",0x") +
                              QString::number(debugAddress + 0xFFFF00, 16) +
                              QStringLiteral("\n"));
                break;
        }
    }

    // continue emulation
    inDebugger = false;
}

void MainWindow::debuggerProcessCommand(int reason, uint32_t input) {
    int row = 0;

    // This means the program is trying to send us a debug command. Let's see what we can do with that information
    if (reason > NUM_DBG_COMMANDS) {
       debuggerExecuteCommand(static_cast<uint32_t>(reason-DBG_PORT_RANGE), static_cast<uint8_t>(input));
       return;
    }

    // We hit a normal breakpoint; raise the correct entry in the port monitor table
    if (reason == HIT_EXEC_BREAKPOINT) {
        QString input3String = int2hex(input, 6);

        // find the correct entry
        while (ui->breakpointView->item(row++, BREAK_ADDR_LOC)->text() != input3String);

        ui->breakChangeLabel->setText(input3String);
        ui->breakTypeLabel->setText(tr("Executed"));
        ui->breakpointView->selectRow(row-1);
    }

    else if (reason == HIT_READ_BREAKPOINT || reason == HIT_WRITE_BREAKPOINT) {
        QString input3String = int2hex(input, 6);

        // find the correct entry
        while (ui->watchpointView->item(row++, BREAK_ADDR_LOC)->text() != input3String);

        ui->watchChangeLabel->setText(input3String);
        ui->watchTypeLabel->setText((reason == HIT_READ_BREAKPOINT) ? tr("Read") : tr("Write"));
        ui->watchpointView->selectRow(row-1);
        memUpdate(input);
    }

    // We hit a port read or write; raise the correct entry in the port monitor table
    else if (reason == HIT_PORT_READ_BREAKPOINT || reason == HIT_PORT_WRITE_BREAKPOINT) {
        QString input2String = int2hex(input, 4);

        // find the correct entry
        while (ui->portView->item(row++, 0)->text() != input2String);

        ui->portChangeLabel->setText(input2String);
        ui->portTypeLabel->setText((reason == HIT_PORT_READ_BREAKPOINT) ? tr("Read") : tr("Write"));
        ui->portView->selectRow(row-1);
    }

    // Checked everything, now we should raise the debugger
    debuggerRaise();
}

void MainWindow::debuggerUpdateChanges() {
    if (!inDebugger) {
        return;
    }

    /* Update all the changes in the core */
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

    debugger.total_cycles = static_cast<uint64_t>(ui->cycleView->text().toULongLong());

    uint32_t uiPC = static_cast<uint32_t>(hex2int(ui->pcregView->text()));
    if (cpu.registers.PC != uiPC) {
        cpu_flush(uiPC, ui->checkADL->isChecked());
    }

    backlight.brightness = static_cast<uint8_t>(ui->brightnessSlider->value());

    lcd.upbase = static_cast<uint32_t>(hex2int(ui->lcdbaseView->text()));
    lcd.upcurr = static_cast<uint32_t>(hex2int(ui->lcdcurrView->text()));

    lcd.control &= ~14;
    lcd.control |= int2Bpp(ui->bppView->text().toUInt()) << 1;

    set_reset(ui->checkPowered->isChecked(), 0x800, lcd.control);
    set_reset(ui->checkBEPO->isChecked(), 0x400, lcd.control);
    set_reset(ui->checkBEBO->isChecked(), 0x200, lcd.control);
    set_reset(ui->checkBGR->isChecked(), 0x100, lcd.control);
}

void MainWindow::debuggerGUISetState(bool state) {
    if (state) {
        ui->buttonRun->setText(tr("Run"));
        ui->buttonRun->setIcon(runIcon);
    } else {
        ui->buttonRun->setText(tr("Stop"));
        ui->buttonRun->setIcon(stopIcon);
        ui->portChangeLabel->clear();
        ui->portTypeLabel->clear();
        ui->breakChangeLabel->clear();
        ui->breakTypeLabel->clear();
        ui->opView->clear();
        ui->vatView->clear();
        ui->portChangeLabel->clear();
        ui->portTypeLabel->clear();
        ui->breakChangeLabel->clear();
        ui->breakTypeLabel->clear();
        ui->watchChangeLabel->clear();
        ui->watchTypeLabel->clear();
    }
    setReceiveState(false);

    ui->tabDebugging->setEnabled(state);
    ui->buttonGoto->setEnabled(state);
    ui->buttonStepIn->setEnabled(state);
    ui->buttonStepOver->setEnabled(state);
    ui->buttonStepNext->setEnabled(state);
    ui->buttonStepOut->setEnabled(state);
    ui->groupCPU->setEnabled(state);
    ui->groupFlags->setEnabled(state);
    ui->groupRegisters->setEnabled(state);
    ui->groupInterrupts->setEnabled(state);
    ui->groupStack->setEnabled(state);
    ui->groupFlash->setEnabled(state);
    ui->groupRAM->setEnabled(state);
    ui->groupMem->setEnabled(state);
    ui->cycleView->setEnabled(state);
    ui->freqView->setEnabled(state);
    ui->opView->setEnabled(state);
    ui->vatView->setEnabled(state);

    ui->actionRestoreState->setEnabled(!state);
    ui->actionImportCalculatorState->setEnabled(!state);
    ui->buttonSend->setEnabled(!state);
    ui->buttonRefreshList->setEnabled(!state);
    ui->emuVarView->setEnabled(!state);
    ui->buttonReceiveFiles->setEnabled(!state && (isReceiving || isSending));
}

void MainWindow::debuggerChangeState() {
    if (emu.rom.isEmpty()) {
        return;
    }

    if (inDebugger) {
        debuggerGUIDisable();
        debuggerUpdateChanges();
        if (isReceiving || isSending) {
            refreshVariableList();
        }
    }
    emit debuggerSendNewState(!inDebugger);
}

void MainWindow::debuggerGUIPopulate() {
    QString tmp;

    tmp = int2hex(cpu.registers.AF, 4);
    ui->afregView->setPalette(tmp == ui->afregView->text() ? nocolorback : colorback);
    ui->afregView->setText(tmp);

    tmp = int2hex(cpu.registers.HL, 6);
    ui->hlregView->setPalette(tmp == ui->hlregView->text() ? nocolorback : colorback);
    ui->hlregView->setText(tmp);

    tmp = int2hex(cpu.registers.DE, 6);
    ui->deregView->setPalette(tmp == ui->deregView->text() ? nocolorback : colorback);
    ui->deregView->setText(tmp);

    tmp = int2hex(cpu.registers.BC, 6);
    ui->bcregView->setPalette(tmp == ui->bcregView->text() ? nocolorback : colorback);
    ui->bcregView->setText(tmp);

    tmp = int2hex(cpu.registers.IX, 6);
    ui->ixregView->setPalette(tmp == ui->ixregView->text() ? nocolorback : colorback);
    ui->ixregView->setText(tmp);

    tmp = int2hex(cpu.registers.IY, 6);
    ui->iyregView->setPalette(tmp == ui->iyregView->text() ? nocolorback : colorback);
    ui->iyregView->setText(tmp);

    tmp = int2hex(cpu.registers._AF, 4);
    ui->af_regView->setPalette(tmp == ui->af_regView->text() ? nocolorback : colorback);
    ui->af_regView->setText(tmp);

    tmp = int2hex(cpu.registers._HL, 6);
    ui->hl_regView->setPalette(tmp == ui->hl_regView->text() ? nocolorback : colorback);
    ui->hl_regView->setText(tmp);

    tmp = int2hex(cpu.registers._DE, 6);
    ui->de_regView->setPalette(tmp == ui->de_regView->text() ? nocolorback : colorback);
    ui->de_regView->setText(tmp);

    tmp = int2hex(cpu.registers._BC, 6);
    ui->bc_regView->setPalette(tmp == ui->bc_regView->text() ? nocolorback : colorback);
    ui->bc_regView->setText(tmp);

    tmp = int2hex(cpu.registers.SPS, 4);
    ui->spsregView->setPalette(tmp == ui->spsregView->text() ? nocolorback : colorback);
    ui->spsregView->setText(tmp);

    tmp = int2hex(cpu.registers.SPL, 6);
    ui->splregView->setPalette(tmp == ui->splregView->text() ? nocolorback : colorback);
    ui->splregView->setText(tmp);

    tmp = int2hex(cpu.registers.MBASE, 2);
    ui->mbregView->setPalette(tmp == ui->mbregView->text() ? nocolorback : colorback);
    ui->mbregView->setText(tmp);

    tmp = int2hex(cpu.registers.I, 4);
    ui->iregView->setPalette(tmp == ui->iregView->text() ? nocolorback : colorback);
    ui->iregView->setText(tmp);

    tmp = int2hex(cpu.IM - !!cpu.IM, 1);
    ui->imregView->setPalette(tmp == ui->imregView->text() ? nocolorback : colorback);
    ui->imregView->setText(tmp);

    tmp = int2hex(cpu.registers.PC, 6);
    ui->pcregView->setPalette(tmp == ui->pcregView->text() ? nocolorback : colorback);
    ui->pcregView->setText(tmp);

    tmp = int2hex(cpu.registers.R >> 1 | cpu.registers.R << 7, 2);
    ui->rregView->setPalette(tmp == ui->rregView->text() ? nocolorback : colorback);
    ui->rregView->setText(tmp);

    tmp = int2hex(lcd.upbase, 6);
    ui->lcdbaseView->setPalette(tmp == ui->lcdbaseView->text() ? nocolorback : colorback);
    ui->lcdbaseView->setText(tmp);

    tmp = int2hex(lcd.upcurr, 6);
    ui->lcdcurrView->setPalette(tmp == ui->lcdcurrView->text() ? nocolorback : colorback);
    ui->lcdcurrView->setText(tmp);

    tmp = QString::number(sched.clockRates[CLOCK_CPU]);
    ui->freqView->setPalette(tmp == ui->freqView->text() ? nocolorback : colorback);
    ui->freqView->setText(tmp);

    tmp = QString::number(debugger.total_cycles);
    ui->cycleView->setPalette(tmp == ui->cycleView->text() ? nocolorback : colorback);
    ui->cycleView->setText(tmp);

    batteryIsCharging(control.batteryCharging);
    batteryChangeStatus(control.setBatteryStatus);

    tmp = bpp2Str((lcd.control >> 1) & 7);
    ui->bppView->setPalette(tmp == ui->bppView->text() ? nocolorback : colorback);
    ui->bppView->setText(tmp);

    ui->check3->setChecked(cpu.registers.flags._3);
    ui->check5->setChecked(cpu.registers.flags._5);
    ui->checkZ->setChecked(cpu.registers.flags.Z);
    ui->checkC->setChecked(cpu.registers.flags.C);
    ui->checkHC->setChecked(cpu.registers.flags.H);
    ui->checkPV->setChecked(cpu.registers.flags.PV);
    ui->checkN->setChecked(cpu.registers.flags.N);
    ui->checkS->setChecked(cpu.registers.flags.S);

    ui->checkADL->setChecked(cpu.ADL);
    ui->checkMADL->setChecked(cpu.MADL);
    ui->checkHalted->setChecked(cpu.halted);
    ui->checkIEF1->setChecked(cpu.IEF1);
    ui->checkIEF2->setChecked(cpu.IEF2);

    ui->checkPowered->setChecked(lcd.control & 0x800);
    ui->checkBEPO->setChecked(lcd.control & 0x400);
    ui->checkBEBO->setChecked(lcd.control & 0x200);
    ui->checkBGR->setChecked(lcd.control & 0x100);
    ui->brightnessSlider->setValue(backlight.brightness);

    ui->portView->blockSignals(true);
    ui->watchpointView->blockSignals(true);

    for (int i = 0; i < ui->portView->rowCount(); i++) {
        portUpdate(i);
    }

    for (int i = 0; i < ui->watchpointView->rowCount(); i++) {
        watchpointUpdate(i);
    }

    updateTIOSView();
    updateStackView();
    prevDisasmAddress = cpu.registers.PC;
    updateDisasmView(prevDisasmAddress, true);

    ramUpdate();
    flashUpdate();
    memUpdate(prevDisasmAddress);
    ui->portView->blockSignals(false);
    ui->watchpointView->blockSignals(false);
}

// ------------------------------------------------
// Clock items
// ------------------------------------------------

void MainWindow::debuggerZeroClockCounter() {
    debugger.total_cycles = 0;
    ui->cycleView->setText("0");
}

// ------------------------------------------------
// Breakpoints
// ------------------------------------------------

void MainWindow::breakpointSetPreviousAddress(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }
    prevBreakpointAddress = static_cast<uint32_t>(hex2int(ui->breakpointView->item(curr_item->row(), BREAK_ADDR_LOC)->text()));
}

bool MainWindow::breakpointRemoveSelectedRow() {
    if (!ui->breakpointView->rowCount() || !ui->breakpointView->selectionModel()->isSelected(ui->breakpointView->currentIndex())) {
        return false;
    }

    const int currRow = ui->breakpointView->currentRow();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(currRow, BREAK_ADDR_LOC)->text()));

    debug_breakwatch(address, DBG_EXEC_BREAKPOINT, false);

    ui->breakpointView->removeRow(currRow);
    return true;
}

void MainWindow::breakpointRemoveAddress(uint32_t address) {
    for (int i = 0; i < ui->breakpointView->rowCount(); i++) {
        if (ui->breakpointView->item(i, BREAK_ADDR_LOC)->text().toUInt() == address) {
            ui->breakpointView->selectRow(i);
            breakpointRemoveSelectedRow();
            break;
        }
    }
}

void MainWindow::breakpointDataChanged(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();
    QString addressString;
    uint32_t address;

    if (col == BREAK_ENABLE_LOC) {
        address = static_cast<uint32_t>(hex2int(ui->breakpointView->item(row, BREAK_ADDR_LOC)->text()));
        debug_breakwatch(address, DBG_EXEC_BREAKPOINT, item->checkState() == Qt::Checked);
    } else if (col == BREAK_ADDR_LOC){
        std::string s = item->text().toUpper().toStdString();
        unsigned mask;

        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty() || s.length() > 6) {
            item->setText(int2hex(prevBreakpointAddress, 6));
            return;
        }

        address = static_cast<uint32_t>(hex2int(QString::fromStdString(s)));
        addressString = int2hex(address, 6);

        ui->breakpointView->blockSignals(true);

        // Return if address is already set
        for (int i = 0; i < ui->breakpointView->rowCount(); i++) {
            if (ui->breakpointView->item(i, BREAK_ADDR_LOC)->text() == addressString && i != row) {
                item->setText(int2hex(prevBreakpointAddress, 6));
                ui->breakpointView->blockSignals(false);
                return;
            }
        }

        mask = ((ui->breakpointView->item(row, BREAK_ENABLE_LOC)->checkState() == Qt::Checked) ? DBG_EXEC_BREAKPOINT
                                                                                               : DBG_NO_HANDLE);

        debug_breakwatch(prevBreakpointAddress, DBG_EXEC_BREAKPOINT, false);
        item->setText(addressString);
        debug_breakwatch(address, mask, true);
        ui->breakpointView->blockSignals(false);
    }
}

QString MainWindow::breakpointNextLabel() {
    return QStringLiteral("Label")+QString::number(ui->breakpointView->rowCount());
}

QString MainWindow::watchpointNextLabel() {
    return QStringLiteral("Label")+QString::number(ui->watchpointView->rowCount());
}

void MainWindow::breakpointSlotAdd(void) {
    breakpointAdd(breakpointNextLabel(), 0, true);
}

void MainWindow::breakpointGUIAdd() {
    uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));

    QTextCursor c = ui->disassemblyView->textCursor();
    c.setCharFormat(ui->disassemblyView->currentCharFormat());

    if (!breakpointAdd(breakpointNextLabel(), address, true)) {
        breakpointRemoveSelectedRow();
    }

    disasm.base_address = address;
    disasmHighlight.hit_exec_breakpoint = false;
    disassembleInstruction();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+9, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the red dot
    if (disasmHighlight.hit_exec_breakpoint) {
        c.insertHtml("<font color='#FFA3A3'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }
}

bool MainWindow::breakpointAdd(QString label, uint32_t address, bool enabled) {
    const int currRow = ui->breakpointView->rowCount();
    QString addressStr = int2hex((address &= 0xFFFFFF), 6).toUpper();

    // Return if address is already set
    for (int i = 0; i < currRow; i++) {
        if (ui->breakpointView->item(i, BREAK_ADDR_LOC)->text() == addressStr) {
            if (addressStr != "000000") {
                ui->breakpointView->selectRow(i);
                return false;
            }
        }
    }

    ui->breakpointView->setUpdatesEnabled(false);
    ui->breakpointView->blockSignals(true);

    ui->breakpointView->setRowCount(currRow + 1);

    QTableWidgetItem *itemLabel   = new QTableWidgetItem(label);
    QTableWidgetItem *itemAddress = new QTableWidgetItem(addressStr);
    QTableWidgetItem *itemBreak   = new QTableWidgetItem();

    itemBreak->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
    itemBreak->setFlags(itemBreak->flags() & ~Qt::ItemIsEditable);

    ui->breakpointView->setItem(currRow, BREAK_LABEL_LOC, itemLabel);
    ui->breakpointView->setItem(currRow, BREAK_ADDR_LOC, itemAddress);
    ui->breakpointView->setItem(currRow, BREAK_ENABLE_LOC, itemBreak);

    ui->breakpointView->setCurrentCell(currRow, BREAK_ADDR_LOC);
    ui->breakpointView->setUpdatesEnabled(true);

    debug_breakwatch(address, DBG_EXEC_BREAKPOINT, enabled);

    prevBreakpointAddress = address;
    ui->breakpointView->blockSignals(false);
    return true;
}

// ------------------------------------------------
// Ports
// ------------------------------------------------

void MainWindow::portSetPreviousAddress(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }
    prevPortAddress = static_cast<uint16_t>(hex2int(ui->portView->item(curr_item->row(), PORT_ADDR_LOC)->text()));
}

void MainWindow::portRemoveSelected() {
    if (!ui->portView->rowCount() || !ui->portView->selectionModel()->isSelected(ui->portView->currentIndex())) {
        return;
    }

    const int currRow = ui->portView->currentRow();
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currRow, PORT_ADDR_LOC)->text()));

    debug_pmonitor_remove(port);
    ui->portView->removeRow(currRow);
}


void MainWindow::portUpdate(int currRow) {
    uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(currRow, PORT_ADDR_LOC)->text()));
    uint8_t read = static_cast<uint8_t>(port_peek_byte(port));

    ui->portView->item(currRow, PORT_VALUE_LOC)->setText(int2hex(read, 2));
}

void MainWindow::portSlotAdd(void) {
    portAdd(0, DBG_NO_HANDLE);
}

bool MainWindow::portAdd(uint16_t port, unsigned mask) {
    const int currRow = ui->portView->rowCount();
    QString portStr = int2hex((port &= 0xFFFF), 4).toUpper();
    uint8_t data;

    // Read the data from the port
    data = port_peek_byte(port);

    // Return if port is already set
    for (int i = 0; i < currRow; i++) {
        if (ui->portView->item(i, PORT_ADDR_LOC)->text() == portStr) {
            if (portStr != "0000") {
                return false;
            }
        }
    }

    ui->portView->setRowCount(currRow + 1);
    ui->portView->setUpdatesEnabled(false);
    ui->portView->blockSignals(true);

    QTableWidgetItem *itemAddress = new QTableWidgetItem(portStr);
    QTableWidgetItem *itemData    = new QTableWidgetItem(int2hex(data, 2));
    QTableWidgetItem *itemRBreak  = new QTableWidgetItem();
    QTableWidgetItem *itemWBreak  = new QTableWidgetItem();
    QTableWidgetItem *itemFreeze  = new QTableWidgetItem();

    itemRBreak->setFlags(itemRBreak->flags() & ~Qt::ItemIsEditable);
    itemWBreak->setFlags(itemWBreak->flags() & ~Qt::ItemIsEditable);
    itemFreeze->setFlags(itemFreeze->flags() & ~Qt::ItemIsEditable);

    itemRBreak->setCheckState((mask & DBG_PORT_READ) ? Qt::Checked : Qt::Unchecked);
    itemWBreak->setCheckState((mask & DBG_PORT_WRITE) ? Qt::Checked : Qt::Unchecked);
    itemFreeze->setCheckState((mask & DBG_PORT_FREEZE) ? Qt::Checked : Qt::Unchecked);

    ui->portView->setItem(currRow, PORT_ADDR_LOC, itemAddress);
    ui->portView->setItem(currRow, PORT_VALUE_LOC, itemData);
    ui->portView->setItem(currRow, PORT_READ_LOC, itemRBreak);
    ui->portView->setItem(currRow, PORT_WRITE_LOC, itemWBreak);
    ui->portView->setItem(currRow, PORT_FREEZE_LOC, itemFreeze);

    ui->portView->selectRow(currRow);
    ui->portView->setUpdatesEnabled(true);
    prevPortAddress = port;
    ui->portView->blockSignals(false);
    return true;
}

void MainWindow::portDataChanged(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();

    if (col == PORT_READ_LOC || col == PORT_WRITE_LOC || col == PORT_FREEZE_LOC) {
        uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(row, PORT_ADDR_LOC)->text()));
        unsigned int mask = DBG_NO_HANDLE;

        if (col == PORT_READ_LOC) {   // Break on read
            mask = DBG_PORT_READ;
        }
        if (col == PORT_WRITE_LOC) {  // Break on write
            mask = DBG_PORT_WRITE;
        }
        if (col == PORT_FREEZE_LOC) { // Freeze
            mask = DBG_PORT_FREEZE;
        }
        debug_pmonitor_set(port, mask, item->checkState() == Qt::Checked);
    } else if (col == PORT_ADDR_LOC) {
        std::string s = item->text().toUpper().toStdString();
        unsigned mask;

        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty() || s.length() > 4) {
            item->setText(int2hex(prevPortAddress, 4));
            return;
        }

        uint16_t port = static_cast<uint16_t>(hex2int(QString::fromStdString(s)));
        uint8_t data = port_peek_byte(port);
        QString portStr = int2hex(port, 4);

        ui->portView->blockSignals(true);

        // Return if port is already set
        for (int i=0; i<ui->portView->rowCount(); i++) {
            if (ui->portView->item(i, PORT_ADDR_LOC)->text() == portStr && i != row) {
                item->setText(int2hex(prevPortAddress, 4));
                ui->portView->blockSignals(false);
                return;
            }
        }

        debug_pmonitor_remove(prevPortAddress);

        mask = ((ui->portView->item(row, PORT_READ_LOC)->checkState() == Qt::Checked) ? DBG_PORT_READ : DBG_NO_HANDLE)  |
               ((ui->portView->item(row, PORT_WRITE_LOC)->checkState() == Qt::Checked) ? DBG_PORT_WRITE : DBG_NO_HANDLE) |
               ((ui->portView->item(row, PORT_FREEZE_LOC)->checkState() == Qt::Checked) ? DBG_PORT_FREEZE : DBG_NO_HANDLE);

        debug_pmonitor_set(port, mask, true);
        item->setText(portStr);
        ui->portView->item(row, PORT_VALUE_LOC)->setText(int2hex(data, 2));
    } else if (col == PORT_VALUE_LOC) {
        uint8_t pdata = static_cast<uint8_t>(hex2int(item->text()));
        uint16_t port = static_cast<uint16_t>(hex2int(ui->portView->item(row, PORT_ADDR_LOC)->text()));

        port_poke_byte(port, pdata);

        item->setText(int2hex(port_peek_byte(port), 2));
    }
    ui->portView->blockSignals(false);
}

// ------------------------------------------------
// Watchpoints
// ------------------------------------------------

void MainWindow::watchpointSetPreviousAddress(QTableWidgetItem *curr_item) {
    if (curr_item->text().isEmpty()) {
        return;
    }

    prevWatchpointAddress = static_cast<uint32_t>(hex2int(ui->watchpointView->item(curr_item->row(), WATCH_ADDR_LOC)->text()));
}

bool MainWindow::watchpointRemoveSelectedRow() {
    if (!ui->watchpointView->rowCount() || !ui->watchpointView->selectionModel()->isSelected(ui->watchpointView->currentIndex())) {
        return false;
    }

    const int currRow = ui->watchpointView->currentRow();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(currRow, WATCH_ADDR_LOC)->text()));

    debug_breakwatch(address, DBG_READ_WATCHPOINT | DBG_WRITE_WATCHPOINT, false);

    ui->watchpointView->removeRow(currRow);
    return true;
}

void MainWindow::watchpointRemoveAddress(uint32_t address) {
    for (int i = 0; i < ui->watchpointView->rowCount(); i++) {
        if (ui->watchpointView->item(i, WATCH_ADDR_LOC)->text().toUInt() == address) {
            ui->watchpointView->selectRow(i);
            watchpointRemoveSelectedRow();
            break;
        }
    }
}

void MainWindow::watchpointUpdate(int currRow) {
    uint8_t i,size = ui->watchpointView->item(currRow, WATCH_SIZE_LOC)->text().toUInt();
    uint32_t address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(currRow, WATCH_ADDR_LOC)->text()));
    uint32_t read = 0;

    for (i=0; i<size; i++) {
        read |= mem_peek_byte(address+i) << (i << 3);
    }

    ui->watchpointView->item(currRow, WATCH_VALUE_LOC)->setText(int2hex(read, size << 1));
}

void MainWindow::watchpointReadGUIAdd() {
    watchpointGUIMask = DBG_READ_WATCHPOINT;
    watchpointGUIAdd();
}

void MainWindow::watchpointWriteGUIAdd() {
    watchpointGUIMask = DBG_WRITE_WATCHPOINT;
    watchpointGUIAdd();
}

void MainWindow::watchpointReadWriteGUIAdd() {
    watchpointGUIMask = DBG_RW_WATCHPOINT;
    watchpointGUIAdd();
}

void MainWindow::watchpointGUIAdd() {
    unsigned mask = watchpointGUIMask;
    uint32_t address = static_cast<uint32_t>(hex2int(ui->disassemblyView->getSelectedAddress()));

    QTextCursor c = ui->disassemblyView->textCursor();
    c.setCharFormat(ui->disassemblyView->currentCharFormat());

    if (!watchpointAdd(watchpointNextLabel(), address, 1, mask)) {
        watchpointRemoveSelectedRow();
    }

    disasm.base_address = address;
    disasmHighlight.hit_read_watchpoint = false;
    disasmHighlight.hit_write_watchpoint = false;
    disassembleInstruction();

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+7, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the green dot
    if (disasmHighlight.hit_read_watchpoint) {
        c.insertHtml("<font color='#A3FFA3'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }

    c.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    c.setPosition(c.position()+8, QTextCursor::MoveAnchor);
    c.deleteChar();

    // Add the blue dot
    if (disasmHighlight.hit_write_watchpoint) {
        c.insertHtml("<font color='#A3A3FF'>&#9679;</font>");
    } else {
        c.insertText(" ");
    }
}

void MainWindow::watchpointSlotAdd(void) {
    watchpointAdd(watchpointNextLabel(), 0, 1, DBG_READ_WATCHPOINT | DBG_WRITE_WATCHPOINT);
}

bool MainWindow::watchpointAdd(QString label, uint32_t address, uint8_t len, unsigned mask) {
    const int currRow = ui->watchpointView->rowCount();
    QString addressStr = int2hex((address &= 0xFFFFFF), 6).toUpper();
    QString watchLen;

    if (!len) { len = 1; }
    if (len > 4) { len = 4; }
    watchLen = QString::number(len);

    // Return if address is already set
    for (int i = 0; i < currRow; i++) {
        if (ui->watchpointView->item(i, WATCH_ADDR_LOC)->text() == addressStr) {
            if (addressStr != "000000") {
                ui->watchpointView->selectRow(i);
                return false;
            }
        }
    }

    ui->watchpointView->setUpdatesEnabled(false);
    ui->watchpointView->blockSignals(true);

    QTableWidgetItem *itemLabel   = new QTableWidgetItem(label);
    QTableWidgetItem *itemAddress = new QTableWidgetItem(addressStr);
    QTableWidgetItem *itemSize    = new QTableWidgetItem(watchLen);
    QTableWidgetItem *itemData    = new QTableWidgetItem();
    QTableWidgetItem *itemRWatch  = new QTableWidgetItem();
    QTableWidgetItem *itemWWatch  = new QTableWidgetItem();

    itemWWatch->setFlags(itemWWatch->flags() & ~Qt::ItemIsEditable);
    itemRWatch->setFlags(itemRWatch->flags() & ~Qt::ItemIsEditable);

    itemRWatch->setCheckState((mask & DBG_READ_WATCHPOINT) ? Qt::Checked : Qt::Unchecked);
    itemWWatch->setCheckState((mask & DBG_WRITE_WATCHPOINT) ? Qt::Checked : Qt::Unchecked);

    ui->watchpointView->setRowCount(currRow + 1);

    ui->watchpointView->setItem(currRow, WATCH_LABEL_LOC, itemLabel);
    ui->watchpointView->setItem(currRow, WATCH_ADDR_LOC,  itemAddress);
    ui->watchpointView->setItem(currRow, WATCH_SIZE_LOC,  itemSize);
    ui->watchpointView->setItem(currRow, WATCH_VALUE_LOC, itemData);
    ui->watchpointView->setItem(currRow, WATCH_READ_LOC,  itemRWatch);
    ui->watchpointView->setItem(currRow, WATCH_WRITE_LOC, itemWWatch);

    watchpointUpdate(currRow);

    ui->watchpointView->setCurrentCell(currRow, WATCH_ADDR_LOC);
    ui->watchpointView->setUpdatesEnabled(true);

    // actually set it in the debugger core
    debug_breakwatch(address, mask, true);

    prevWatchpointAddress = address;
    ui->watchpointView->blockSignals(false);
    return true;
}

void MainWindow::watchpointDataChanged(QTableWidgetItem *item) {
    auto row = item->row();
    auto col = item->column();
    QString newString;
    uint32_t address;

    ui->watchpointView->blockSignals(true);

    if (col == WATCH_VALUE_LOC) { // update the data located at this address
        uint8_t i,wSize = ui->watchpointView->item(row, WATCH_SIZE_LOC)->text().toUInt();
        uint32_t wData;

        address = static_cast<uint32_t>(ui->watchpointView->item(row, WATCH_ADDR_LOC)->text().toUInt());

        std::string s = item->text().toUpper().toStdString();
        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty()) {
            item->setText(int2hex(0, wSize << 1));
            ui->watchpointView->blockSignals(false);
            return;
        }

        wData = static_cast<uint32_t>(item->text().toUpper().toUInt(Q_NULLPTR, 16));
        newString = int2hex(wData, wSize << 1);

        item->setText(newString);
        wData = static_cast<uint32_t>(newString.toUInt(Q_NULLPTR, 16));

        for (i = 0; i < wSize; i++) {
            mem_poke_byte(address + i, (wData >> ((i << 3)) & 255));
        }

        ramUpdate();
        flashUpdate();
        memUpdate(address);
    } else if (col == WATCH_SIZE_LOC) { // length of data we wish to read
        unsigned int data_length = item->text().toUInt();
        if (data_length > 4) {
            data_length = 4;
        } else if (data_length < 1) {
            data_length = 1;
        }
        item->setText(QString::number(data_length));
        watchpointUpdate(row);
    } else if (col == WATCH_READ_LOC || col == WATCH_WRITE_LOC) {
        address = static_cast<uint32_t>(hex2int(ui->watchpointView->item(row, WATCH_ADDR_LOC)->text()));
        unsigned int value = DBG_NO_HANDLE;

        if (col == WATCH_READ_LOC) { // Break on read
            value = DBG_READ_WATCHPOINT;
        } else
        if (col == WATCH_WRITE_LOC) { // Break on write
            value = DBG_WRITE_WATCHPOINT;
        }
        debug_breakwatch(address, value, item->checkState() == Qt::Checked);
    } else if (col == WATCH_ADDR_LOC){
        std::string s = item->text().toUpper().toStdString();
        unsigned mask;

        if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos || s.empty() || s.length() > 6) {
            item->setText(int2hex(prevWatchpointAddress, 6));
            ui->watchpointView->blockSignals(false);
            return;
        }

        address = static_cast<uint32_t>(hex2int(item->text().toUpper()));
        newString = int2hex(address, 6);

        /* Return if address is already set */
        for (int i = 0; i < ui->watchpointView->rowCount(); i++) {
            if (ui->watchpointView->item(i, WATCH_ADDR_LOC)->text() == newString && i != row) {
                item->setText(int2hex(prevWatchpointAddress, 6));
                ui->watchpointView->blockSignals(false);
                return;
            }
        }

        mask = ((ui->watchpointView->item(row, WATCH_READ_LOC)->checkState() == Qt::Checked) ? DBG_READ_WATCHPOINT : DBG_NO_HANDLE)|
               ((ui->watchpointView->item(row, WATCH_WRITE_LOC)->checkState() == Qt::Checked) ? DBG_WRITE_WATCHPOINT : DBG_NO_HANDLE);

        debug_breakwatch(prevWatchpointAddress, DBG_RW_WATCHPOINT, false);
        item->setText(newString);
        debug_breakwatch(address, mask, true);
        watchpointUpdate(row);
    }
    ui->watchpointView->blockSignals(false);
}

// ------------------------------------------------
// Battery Status
// ------------------------------------------------

void MainWindow::batteryIsCharging(bool checked) {
    control.batteryCharging = checked;
}

void MainWindow::batteryChangeStatus(int value) {
    control.setBatteryStatus = static_cast<uint8_t>(value);
    ui->sliderBattery->setValue(value);
    ui->labelBattery->setText(QString::number(value * 20) + "%");
}

// ------------------------------------------------
// Disassembly View
// ------------------------------------------------

void MainWindow::scrollDisasmView(int value) {
    if (value >= ui->disassemblyView->verticalScrollBar()->maximum()) {
        ui->disassemblyView->verticalScrollBar()->blockSignals(true);
        drawNextDisassembleLine();
        ui->disassemblyView->verticalScrollBar()->setValue(ui->disassemblyView->verticalScrollBar()->maximum()-1);
        ui->disassemblyView->verticalScrollBar()->blockSignals(false);
    }
}

void MainWindow::equatesClear() {
    // Reset the map
    currentEquateFile.clear();
    disasm.addressMap.clear();
    QMessageBox::warning(this, tr("Equates Cleared"), tr("Cleared disassembly equates."));
    updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(Q_NULLPTR, 16), true);
}

void MainWindow::equatesRefresh() {
    // Reset the map
    if (fileExists(currentEquateFile.toStdString())) {
        disasm.addressMap.clear();
        equatesAddFile(currentEquateFile);
        updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(Q_NULLPTR, 16), true);
    } else {
        QMessageBox::warning(this, tr("Error Opening"), tr("Couldn't open equates file."));
    }
}

void MainWindow::equatesAddDialog() {
    QFileDialog dialog(this);
    int good;

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(currentDir);

    QStringList extFilters;
    extFilters << tr("Equate files (*.inc *.lab)")
               << tr("All Files (*.*)");
    dialog.setNameFilters(extFilters);

    good = dialog.exec();
    currentDir = dialog.directory();

    if (!good) { return; }

    equatesAddFile(dialog.selectedFiles().first());
}

void MainWindow::equatesAddFile(QString fileName) {
    std::string current;
    std::ifstream in;
    currentEquateFile = fileName;
    in.open(fileName.toStdString());

    if (in.good()) {
        QRegularExpression equatesRegexp("^\\h*([^\\W\\d]\\w*)\\h*(?:=|\\h\\.?equ(?!\\d))\\h*(?|\\$([\\da-f]{4,})|(\\d[\\da-f]{3,})h)\\h*(?:;.*)?$",
                                         QRegularExpression::CaseInsensitiveOption);
        // Reset the map
        disasm.addressMap.clear();
        while (std::getline(in, current)) {
            QRegularExpressionMatch matches = equatesRegexp.match(QString::fromStdString(current));
            if (matches.hasMatch()) {
                uint32_t address = static_cast<uint32_t>(matches.capturedRef(2).toUInt(Q_NULLPTR, 16));
                std::string &item = disasm.addressMap[address];
                if (item.empty()) {
                    item = matches.captured(1).toStdString();
                    uint8_t *ptr = phys_mem_ptr(address - 4, 9);
                    if (ptr && ptr[4] == 0xC3 && (ptr[0] == 0xC3 || ptr[8] == 0xC3)) { // jump table?
                        uint32_t address2  = ptr[5] | ptr[6] << 8 | ptr[7] << 16;
                        if (phys_mem_ptr(address2, 1)) {
                            std::string &item2 = disasm.addressMap[address2];
                            if (item2.empty()) {
                                item2 = "_" + item;
                            }
                        }
                    }
                }
            }
        }
        in.close();
        updateDisasmView(ui->disassemblyView->getSelectedAddress().toInt(Q_NULLPTR, 16), true);
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Couldn't open this file"));
    }
}

void MainWindow::updateDisasmView(const int sentBase, const bool newPane) {
    addressPane = sentBase;
    fromPane = newPane;
    disasmOffsetSet = false;
    disasm.adl = ui->checkADL->isChecked();
    disasm.base_address = -1;
    disasm.new_address = addressPane - ((newPane) ? 0x40 : 0);
    if (disasm.new_address < 0) { disasm.new_address = 0; }
    int32_t last_address = disasm.new_address + 0x120;
    if (last_address > 0xFFFFFF) { last_address = 0xFFFFFF; }

    disconnect(ui->disassemblyView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::scrollDisasmView);
    ui->disassemblyView->clear();
    ui->disassemblyView->cursorState(false);
    ui->disassemblyView->clearAllHighlights();

    while (disasm.new_address < last_address) {
        drawNextDisassembleLine();
    }

    ui->disassemblyView->setTextCursor(disasmOffset);
    ui->disassemblyView->cursorState(true);
    ui->disassemblyView->updateAllHighlights();
    ui->disassemblyView->centerCursor();
    connect(ui->disassemblyView->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::scrollDisasmView);
}

// ------------------------------------------------
// Misc
// ------------------------------------------------

void MainWindow::debuggerTabSwitched(int tab) {
    if (tab == 0) {
        updateDisasmView(prevDisasmAddress, true);
    } else {
        ui->portView->clearSelection();
        ui->breakpointView->clearSelection();
        ui->watchpointView->clearSelection();
        prevDisasmAddress = ui->disassemblyView->getSelectedAddress().toUInt(Q_NULLPTR, 16);
    }
}

void MainWindow::gotoPressed() {
    bool accept;

    if (prevGotoAddress.isEmpty()) {
        prevGotoAddress = ui->disassemblyView->getSelectedAddress();
    }

    QString address = getAddressString(prevGotoAddress, &accept);

    if (accept) {
        updateDisasmView(hex2int(prevGotoAddress = address), false);
    }
}
