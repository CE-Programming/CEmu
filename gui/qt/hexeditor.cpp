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
    ui->ramEdit->setBase(0xD00000);
    ui->ramEdit->setData(QByteArray::fromRawData(reinterpret_cast<char*>(mem.ram.block), 0x65800));
}

void MainWindow::memUpdateEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    QByteArray data;
    int off = edit->getOffset();
    int base = edit->getBase();
    int mid = off + base;
    int start = mid - 0x1000;
    int end = mid + 0x1000;

    if (start < 0) {
        start = 0;
    } else {
        if (start < 0x1000) {
            if (off > 0x1000) {
                off -= start;
            }
        }
    }
    if (end > 0xFFFFFF) {
        end = 0xFFFFFF;
    }
    data.resize(end - start + 1);

    fprintf(stdout, "base: 0x%06X\nstart: 0x%06X\noffset: 0x%06X\n", base, start, off);
    fflush(stdout);

    for (int j = 0, i = start; i < end; j++, i++) {
        data[j] = mem_peek_byte(i);
    }

    edit->setBase(start);
    edit->setData(data);
    edit->setOffset(off);
}

void MainWindow::flashGotoPressed() {
    bool accept = false;
    QString addrStr = getAddressString(m_flashGotoAddr, &accept);

    if (accept) {
        m_flashGotoAddr = addrStr;
        ui->flashEdit->setFocus();
        ui->flashEdit->setOffset(hex2int(addrStr));
    }
}

void MainWindow::ramGotoPressed() {
    bool accept = false;
    QString addrStr = getAddressString(m_RamGotoAddr, &accept);

    if (accept) {
        m_RamGotoAddr = addrStr;
        ui->ramEdit->setFocus();
        ui->ramEdit->setOffset(hex2int(addrStr) - 0xD00000);
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

    for (int j = 0, i = start; i < end; i++, j++) {
        data[j] = mem_peek_byte(i);
    }

    edit->setFocus();
    edit->setData(data);
    edit->setBase(start);
    edit->setOffset(addr - start);
}

void MainWindow::memGotoEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    bool accept = false;
    QString address = getAddressString(m_memGotoAddr, &accept);

    if (accept) {
        memGoto(edit, static_cast<uint32_t>(hex2int(m_memGotoAddr = address)));
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
    if (ui->flashEdit->modifiedCount()) {
        memcpy(mem.flash.block, ui->flashEdit->data(), 0x400000);
    }
    syncHexWidget(ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    if (ui->ramEdit->modifiedCount()) {
        memcpy(mem.ram.block, ui->ramEdit->data(), 0x65800);
    }
    syncHexWidget(ui->ramEdit);
}

void MainWindow::memSyncEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    int base = edit->getBase();
    int count = edit->modifiedCount();
    for (int i = 0; count && i < edit->size(); i++) {
        if (edit->modified()[i]) {
            mem_poke_byte(static_cast<uint32_t>(base + i), edit->data()[i]);
            count--;
        }
        qApp->processEvents();
    }

    syncHexWidget(edit);
}

void MainWindow::memAsciiToggle(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    edit->setAsciiArea(!edit->asciiArea());
}
