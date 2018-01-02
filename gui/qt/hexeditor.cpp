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
#include "../../core/mem.h"

// ------------------------------------------------
// Hex Editor Things
// ------------------------------------------------

QString MainWindow::getAddressString(const QString& string, bool* ok) {
    QString address = QInputDialog::getText(this, tr("Goto"),
                                         tr("Input Address (Or Equate):"), QLineEdit::Normal,
                                         string, ok).toUpper();

    QString exists = getAddressEquate(address.toStdString());
    if (!exists.isEmpty()) {
        return exists;
    }

   return int2hex(hex2int(address), 6);
}

QString MainWindow::getAddressEquate(const std::string &in) {
    QString value;
    map_value_t::const_iterator item = disasm.reverseMap.find(in);
    if (item != disasm.reverseMap.end()) {
        value = int2hex(item->second, 6);
    }
    return value;
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

void MainWindow::memUpdate(int index, uint32_t addressBegin) {
    memEditUpdate(memory.at(index), addressBegin);
}

void MainWindow::memEditUpdate(QHexEdit *edit, uint32_t addressBegin) {
    edit->setFocus();
    QByteArray mem_data;

    bool locked = ui->checkLockPosition->isChecked() || edit != ui->memEdit;
    int32_t start, line = 0;

    if (locked) {
        start = static_cast<int32_t>(edit->addressOffset());
        line = edit->getLine();
    } else {
        start = static_cast<int32_t>(addressBegin) - 0x1000;
    }

    if (start < 0) { start = 0; }
    int32_t end = start+0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    edit->memSize = end-start;

    for (int32_t i=start; i<end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    edit->setData(mem_data);
    edit->setAddressOffset(start);

    if (locked) {
        edit->setLine(line);
    } else {
        edit->setCursorPosition((addressBegin-start)<<1);
        edit->ensureVisible();
    }
}

void MainWindow::searchEdit(int index) {
    QHexEdit *edit = memory.at(index);

    SearchWidget search(searchingString, hexSearch);
    int searchMode, err = 0;
    search.show();

    if (!((searchMode = search.exec()) > SEARCH_CANCEL)) { return; }

    hexSearch = search.getType();
    searchingString = search.getSearchString();

    QString searchString;
    if (hexSearch == SEARCH_MODE_HEX) {
        searchString = searchingString;
    } else {
        searchString = QString::fromStdString(searchingString.toLatin1().toHex().toStdString());
    }

    edit->setFocus();
    std::string s = searchString.toUpper().toStdString();
    if (searchString.isEmpty() || (searchString.length() & 1) || s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        QMessageBox::critical(this, MSG_ERROR, tr("Error when reading input string"));
        return;
    }

    QByteArray string_int = QByteArray::fromHex(searchString.toLatin1());

    switch (searchMode) {
        case SEARCH_NEXT_NOT:
            err = edit->indexNotOf(string_int, edit->cursorPosition());
            break;
        case SEARCH_PREV:
            err = edit->indexPrevOf(string_int, edit->cursorPosition());
            break;
        case SEARCH_PREV_NOT:
            err = edit->indexPrevNotOf(string_int, edit->cursorPosition());
            break;
        case SEARCH_NEXT:
        default:
            err = edit->indexOf(string_int, edit->cursorPosition());
            break;
    }

    if (err == -1) {
         QMessageBox::warning(this, MSG_WARNING, tr("String not found."));
    }
}

void MainWindow::flashGotoPressed() {
    bool accept = false;
    QString addressStr = getAddressString(prevFlashAddress, &accept);

    if (accept) {

        ui->flashEdit->setFocus();
        int address = hex2int(addressStr);
        if (address > 0x3FFFFF) {
            return;
        }

        prevFlashAddress = addressStr;

        ui->flashEdit->setCursorPosition(address * 2);
        ui->flashEdit->ensureVisible();
    }
}

void MainWindow::ramGotoPressed() {
    bool accept = false;
    QString addressStr = getAddressString(prevRAMAddress, &accept);

    if (accept) {

        ui->ramEdit->setFocus();
        int address = hex2int(addressStr)-0xD00000;
        if (address >= 0x65800 || address < 0) {
            return;
        }

        prevRAMAddress = addressStr;

        ui->ramEdit->setCursorPosition(address * 2);
        ui->ramEdit->ensureVisible();
    }
}

void MainWindow::memSearchPressed(int index) {
    searchEdit(index);
}

void MainWindow::memGoto(int index, const QString& addressStr) {
    QHexEdit *edit = memory.at(index);

    edit->setFocus();
    int address = hex2int(addressStr);
    if (address > 0xFFFFFF || address < 0) {
        return;
    }

    QByteArray mem_data;
    int start = address-0x500;
    if (start < 0) { start = 0; }
    int end = start+0x1000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    edit->memSize = end-start;

    for (int i = start; i < end; i++) {
        mem_data.append(mem_peek_byte(i));
    }

    edit->setData(mem_data);
    edit->setAddressOffset(start);
    edit->setCursorPosition((address-start) * 2);
    edit->ensureVisible();
}

void MainWindow::memGotoPressed(int index) {
    bool accept = false;
    QString address = getAddressString(prevMemAddress, &accept);

    if (accept) {
        memGoto(index, prevMemAddress = address);
    }
}

void MainWindow::syncHexView(int posa, QHexEdit *edit) {
    debuggerGUIPopulate();
    updateDisasmView(addressPane, fromPane);
    edit->setFocus();
    edit->setCursorPosition(posa);
}

void MainWindow::flashSyncPressed() {
    qint64 posa = ui->flashEdit->cursorPosition();
    memcpy(mem.flash.block, ui->flashEdit->data().data(), 0x400000);
    syncHexView(posa, ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    qint64 posa = ui->ramEdit->cursorPosition();
    memcpy(mem.ram.block, ui->ramEdit->data().data(), 0x65800);
    syncHexView(posa, ui->ramEdit);
}

void MainWindow::memSyncPressed(int index) {
    QHexEdit *edit = memory.at(index);

    int start = edit->addressOffset();
    qint64 posa = edit->cursorPosition();

    for (uint32_t i = 0; i < edit->memSize; i++) {
        mem_poke_byte(i+start, edit->data().at(i));
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    syncHexView(posa, edit);
}
