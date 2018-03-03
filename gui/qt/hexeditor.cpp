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

    QString exists = getAddressOfEquate(address.toUpper().toStdString());
    if (!exists.isEmpty()) {
        return exists;
    }

   return int2hex(hex2int(address), 6);
}

void MainWindow::flashUpdate() {
    ui->flashEdit->setData(QByteArray::fromRawData(reinterpret_cast<char*>(mem.flash.block), 0x400000));
}

void MainWindow::ramUpdate() {
    ui->ramEdit->setBaseAddr(0xD00000);
    ui->ramEdit->setData(QByteArray::fromRawData(reinterpret_cast<char*>(mem.ram.block), 0x65800));
}

void MainWindow::memUpdateEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    QByteArray data;
    int start = edit->baseAddr();
    if (start < 0) { start = 0; }
    int end = start + 0x2000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }

    for (int i = start; i < end; i++) {
        data.append(mem_peek_byte(i));
    }

    edit->setData(data);
}

void MainWindow::flashGotoPressed() {
    bool accept = false;
    QString addrStr = getAddressString(m_flashGotoAddr, &accept);

    if (accept) {
        m_flashGotoAddr = addrStr;
        ui->flashEdit->setFocus();
        ui->flashEdit->setAddr(hex2int(addrStr));
    }
}

void MainWindow::ramGotoPressed() {
    bool accept = false;
    QString addrStr = getAddressString(m_RamGotoAddr, &accept);

    if (accept) {
        m_RamGotoAddr = addrStr;
        ui->ramEdit->setFocus();
        ui->ramEdit->setAddr(hex2int(addrStr) - 0xD00000);
    }
}

void MainWindow::memSearchEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    SearchWidget search(searchingString, hexSearch);
    int searchMode, found = 0;
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

    QByteArray searchBa = QByteArray::fromHex(searchString.toLatin1());

    switch (searchMode) {
        case SEARCH_NEXT_NOT:
            found = edit->indexNotOf(searchBa);
            break;
        case SEARCH_PREV:
            found = edit->indexPrevOf(searchBa);
            break;
        case SEARCH_PREV_NOT:
            found = edit->indexPrevNotOf(searchBa);
            break;
        case SEARCH_NEXT:
        default:
            found = edit->indexOf(searchBa);
            break;
    }

    if (found == -1) {
         QMessageBox::warning(this, MSG_WARNING, tr("String not found."));
    }
}

void MainWindow::memGoto(HexWidget *edit, uint32_t address) {
    if (edit == Q_NULLPTR || !guiDebug || address > 0xFFFFFF) {
        return;
    }

    int addr = static_cast<int>(address);
    int start = addr - 0x500;
    if (start < 0) { start = 0; }
    int end = start + 0x1000;
    if (end > 0xFFFFFF) { end = 0xFFFFFF; }
    QByteArray data;
    data.resize(end - start + 1);

    for (int i = start; i < end; i++) {
        data[i] = mem_peek_byte(i);
    }

    edit->setFocus();
    edit->setData(data);
    edit->setBaseAddr(start);
    edit->setAddr(addr - start);
}

void MainWindow::memGotoEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    bool accept = false;
    QString address = getAddressString(prevMemAddress, &accept);

    if (accept) {
        memGoto(edit, static_cast<uint32_t>(hex2int(prevMemAddress = address)));
    }
}

void MainWindow::syncHexWidget(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    debuggerGUIPopulate();
    updateDisasmAddr(addressPane, fromPane);
    edit->setFocus();
}

void MainWindow::flashSyncPressed() {
    memcpy(mem.flash.block, ui->flashEdit->data(), 0x400000);
    syncHexWidget(ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    memcpy(mem.ram.block, ui->ramEdit->data(), 0x65800);
    syncHexWidget(ui->ramEdit);
}

void MainWindow::memSyncEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    uint32_t start = edit->baseAddr();
    for (int i = 0; i < edit->size(); i++) {
        mem_poke_byte(start + i, edit->data()[i]);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    syncHexWidget(edit);
}

void MainWindow::memAsciiToggle(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    edit->setAsciiArea(!edit->asciiArea());
}
