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

#include "searchwidget.h"
#include "dockwidget.h"
#include "utils.h"

#include "../../core/schedule.h"
#include "../../core/link.h"

// ================================================
// Hex Editor Things
// ================================================

void MainWindow::gotoPressed() {
    bool ok;
    QString address = getAddressString(ok, ui->disassemblyView->getSelectedAddress());

    if (!ok) { return; }

    updateDisasmView(hex2int(address), false);
}


void MainWindow::flashUpdate() {
    ui->flashEdit->setFocus();
    int line = ui->flashEdit->getLine();
    ui->flashEdit->setData(QByteArray::fromRawData((char*)mem.flash.block, 0x400000));
    ui->flashEdit->setLine(line);
}

void MainWindow::ramUpdate() {
    ui->ramEdit->setFocus();
    int line = ui->ramEdit->getLine();
    ui->ramEdit->setData(QByteArray::fromRawData((char*)mem.ram.block, 0x65800));
    ui->ramEdit->setAddressOffset(0xD00000);
    ui->ramEdit->setLine(line);
}

void MainWindow::memUpdate(uint32_t addressBegin) {
    ui->memEdit->setFocus();
    QByteArray mem_data;

    bool locked = ui->checkLockPosition->isChecked();
    int32_t start, line = 0;

    if (locked) {
        start = static_cast<int32_t>(ui->memEdit->addressOffset());
        line = ui->memEdit->getLine();
    } else {
        start = static_cast<int32_t>(addressBegin) - 0x1000;
    }

    if (start < 0) { start = 0; }
    int32_t end = start+0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    memSize = end-start;

    for (int32_t i=start; i<end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);

    if (locked) {
        ui->memEdit->setLine(line);
    } else {
        ui->memEdit->setCursorPosition((addressBegin-start)<<1);
        ui->memEdit->ensureVisible();
    }
}

void MainWindow::searchEdit(QHexEdit *editor) {
    SearchWidget search;
    search.setSearchString(searchingString);
    search.setInputMode(hexSearch);

    search.show();
    search.exec();

    hexSearch = search.getInputMode();
    searchingString = search.getSearchString();

    if (!search.getStatus()) {
        return;
    }

    QString searchString;
    if (hexSearch == true) {
        searchString = searchingString;
    } else {
        searchString = QString::fromStdString(searchingString.toLatin1().toHex().toStdString());
    }

    editor->setFocus();
    std::string s = searchString.toUpper().toStdString();
    if (searchString.isEmpty()) {
        return;
    }
    if ((searchString.length() & 1) || s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        QMessageBox::warning(this, tr("Error"), tr("Error when reading input string"));
        return;
    }

    QByteArray string_int;
    for (int i=0; i<searchString.length(); i+=2) {
        QString a = searchString.at(i);
        a.append(searchString.at(i+1));
        string_int.append(hex2int(a));
    }
    if (editor->indexOf(string_int, editor->cursorPosition()) == -1) {
        QMessageBox::warning(this, tr("Not Found"), tr("Hex string not found."));
    }
}

void MainWindow::flashSearchPressed() {
    searchEdit(ui->flashEdit);
}

void MainWindow::flashGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    ui->flashEdit->setFocus();
    if (!ok) {
        return;
    }
    int int_address = hex2int(address);
    if (int_address > 0x3FFFFF) {
        return;
    }

    ui->flashEdit->setCursorPosition(int_address<<1);
    ui->flashEdit->ensureVisible();
}

void MainWindow::ramSearchPressed() {
    searchEdit(ui->ramEdit);
}

void MainWindow::ramGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    ui->ramEdit->setFocus();
    if (!ok) {
        return;
    }
    int int_address = hex2int(address)-0xD00000;
    if (int_address > 0x657FF || address < 0) {
        return;
    }

    ui->ramEdit->setCursorPosition(int_address<<1);
    ui->ramEdit->ensureVisible();
}
void MainWindow::memSearchPressed() {
    searchEdit(ui->memEdit);
}

void MainWindow::memGoto(QString address) {
    ui->memEdit->setFocus();
    int int_address = hex2int(address);
    if (int_address > 0xFFFFFF || int_address < 0) {
        return;
    }

    QByteArray mem_data;
    int start = int_address-0x500;
    if (start < 0) { start = 0; }
    int end = start+0x1000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    memSize = end-start;

    for (int i=start; i<end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    ui->memEdit->setData(mem_data);
    ui->memEdit->setAddressOffset(start);
    ui->memEdit->setCursorPosition((int_address-start)<<1);
    ui->memEdit->ensureVisible();
}

void MainWindow::memGotoPressed() {
    bool ok;
    QString address = getAddressString(ok, "");

    if (!ok) {
        return;
    }
    memGoto(address);
}

void MainWindow::syncHexView(int posa, QHexEdit *hex_view) {
    populateDebugWindow();
    updateDisasmView(addressPane, fromPane);
    hex_view->setFocus();
    hex_view->setCursorPosition(posa);
}

void MainWindow::flashSyncPressed() {
    qint64 posa = ui->flashEdit->cursorPosition();
    memcpy(mem.flash.block, reinterpret_cast<uint8_t*>(ui->flashEdit->data().data()), 0x400000);
    syncHexView(posa, ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    qint64 posa = ui->ramEdit->cursorPosition();
    memcpy(mem.ram.block, reinterpret_cast<uint8_t*>(ui->ramEdit->data().data()), 0x65800);
    syncHexView(posa, ui->ramEdit);
}

void MainWindow::memSyncPressed() {
    int start = ui->memEdit->addressOffset();
    qint64 posa = ui->memEdit->cursorPosition();

    for (int i = 0; i<memSize; i++) {
        mem_poke_byte(i+start, ui->memEdit->dataAt(i, 1).at(0));
    }

    syncHexView(posa, ui->memEdit);
}
