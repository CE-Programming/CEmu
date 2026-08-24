#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "searchwidget.h"
#include "gotodialog.h"
#include "dockwidget.h"
#include "utils.h"
#include "../../core/schedule.h"
#include "../../core/link.h"
#include "../../core/mem.h"

#include <QtGui/QClipboard>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <algorithm>
#include <QtWidgets/QScrollBar>

#ifdef _MSC_VER
    #include <direct.h>
    #define chdir _chdir
#else
    #include <unistd.h>
#endif

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

   return int2hex(static_cast<uint32_t>(hex2int(address)), 6);
}

void MainWindow::flashUpdate() const {
    if (!guiEmuValid) {
        return;
    }

    ui->flashEdit->setData({reinterpret_cast<const char *>(mem.flash.block), qMin(mem.flash.size, UINT32_C(0xC00000))});
}

void MainWindow::ramUpdate() const {
    if (!guiEmuValid) {
        return;
    }

    ui->ramEdit->setBase(0xD00000);
    ui->ramEdit->setData({reinterpret_cast<const char *>(mem.ram.block), 0x65800});
}

void MainWindow::updateMemoryGuiState(bool debuggerEnabled) const {
    const bool flashViewEnabled = debuggerEnabled || ui->checkFlashLiveRefresh->isChecked();
    const bool ramViewEnabled = debuggerEnabled || ui->checkRamLiveRefresh->isChecked();
    ui->groupFlash->setEnabled(true);
    ui->groupRAM->setEnabled(true);

    ui->buttonFlashGoto->setEnabled(true);
    ui->buttonFlashSearch->setEnabled(debuggerEnabled);
    ui->buttonFlashSync->setEnabled(debuggerEnabled);
    ui->flashDimZeros->setEnabled(true);
    ui->flashDimFFs->setEnabled(true);
    ui->flashAscii->setEnabled(debuggerEnabled);
    ui->flashBytes->setEnabled(debuggerEnabled);
    ui->buttonRamGoto->setEnabled(true);
    ui->buttonRamSearch->setEnabled(debuggerEnabled);
    ui->buttonRamSync->setEnabled(debuggerEnabled);
    ui->ramDimZeros->setEnabled(true);
    ui->ramDimFFs->setEnabled(true);
    ui->ramAscii->setEnabled(debuggerEnabled);
    ui->ramBytes->setEnabled(debuggerEnabled);
    ui->flashEdit->setEnabled(flashViewEnabled);
    ui->ramEdit->setEnabled(ramViewEnabled);
    ui->flashEdit->setReadOnly(!debuggerEnabled);
    ui->ramEdit->setReadOnly(!debuggerEnabled);

    for (const QString &magic : m_docksMemory) {
        QDockWidget *dock = findChild<QDockWidget*>(magic);
        if (dock == Q_NULLPTR) {
            continue;
        }
        if (HexWidget *edit = dock->findChild<HexWidget*>()) {
            const QCheckBox *liveRefresh = dock->findChild<QCheckBox*>(QStringLiteral("checkMemoryLiveRefresh"));
            edit->setEnabled(debuggerEnabled || (liveRefresh != Q_NULLPTR && liveRefresh->isChecked()));
            edit->setReadOnly(!debuggerEnabled);
        }
        for (QPushButton *button : dock->findChildren<QPushButton*>()) {
            button->setEnabled(debuggerEnabled || button->objectName() == QStringLiteral("buttonMemoryGoto"));
        }
        for (QToolButton *tool : dock->findChildren<QToolButton*>()) {
            const QString name = tool->objectName();
            tool->setEnabled(debuggerEnabled || name == QStringLiteral("buttonMemoryDimZeros")
                             || name == QStringLiteral("buttonMemoryDimFFs"));
        }
        for (QSpinBox *spin : dock->findChildren<QSpinBox*>()) {
            spin->setEnabled(debuggerEnabled);
        }
    }
}

void MainWindow::setMemLiveRefresh() {
    ui->flashEdit->setLiveRefreshEnabled(ui->checkFlashLiveRefresh->isChecked());
    ui->ramEdit->setLiveRefreshEnabled(ui->checkRamLiveRefresh->isChecked());
    updateMemoryGuiState(guiDebug);
    bool enabled = ui->checkFlashLiveRefresh->isChecked() || ui->checkRamLiveRefresh->isChecked();
    for (const QString &magic : m_docksMemory) {
        QDockWidget *dock = findChild<QDockWidget*>(magic);
        const QCheckBox *liveRefresh = dock == Q_NULLPTR
            ? Q_NULLPTR : dock->findChild<QCheckBox*>(QStringLiteral("checkMemoryLiveRefresh"));
        HexWidget *edit = dock == Q_NULLPTR ? Q_NULLPTR : dock->findChild<HexWidget*>();
        const bool dockEnabled = liveRefresh != Q_NULLPTR && liveRefresh->isChecked();
        if (edit != Q_NULLPTR) {
            edit->setLiveRefreshEnabled(dockEnabled);
        }
        enabled = enabled || dockEnabled;
    }

    if (!enabled) {
        m_memRefreshTimer.stop();
        return;
    }

    memLiveRefresh();
    m_memRefreshTimer.start();
}

void MainWindow::memLiveRefresh() {
    if (!guiEmuValid) {
        return;
    }

    if (ui->debugMemoryWidget->isVisible()) {
        if (ui->checkFlashLiveRefresh->isChecked()) {
            if (!ui->flashEdit->getSize()) {
                ui->flashEdit->setDataSize(static_cast<int>(qMin(mem.flash.size, UINT32_C(0xC00000))));
            }
            ui->flashEdit->refreshVisibleData();
        }
        if (ui->checkRamLiveRefresh->isChecked()) {
            if (!ui->ramEdit->getSize()) {
                ui->ramEdit->setBase(0xD00000);
                ui->ramEdit->setDataSize(0x65800);
            }
            ui->ramEdit->refreshVisibleData();
        }
    }

    for (const QString &magic : m_docksMemory) {
        QDockWidget *dock = findChild<QDockWidget*>(magic);
        if (dock != Q_NULLPTR && dock->isVisible()) {
            const QCheckBox *liveRefresh = dock->findChild<QCheckBox*>(QStringLiteral("checkMemoryLiveRefresh"));
            if (liveRefresh != Q_NULLPTR && liveRefresh->isChecked()) {
                HexWidget *edit = dock->findChild<HexWidget*>();
                if (edit == Q_NULLPTR) {
                    continue;
                }
                if (!edit->getSize()) {
                    edit->setDataSize(0x1000);
                }
                edit->refreshVisibleData();
            }
        }
    }
}

void MainWindow::memUpdateEdit(HexWidget *edit, bool force) {
    if (edit == Q_NULLPTR || !guiEmuValid) {
        return;
    }

    QByteArray data;

    if (force || edit->getScrolled() || !edit->getSize()) {
        bool second = edit->getCursorOffset() & 1;
        int off = edit->getOffset();
        int base = edit->getBase();
        int addr = off + base;
        int start = addr - 0x1000;
        int end = addr + 0x1000 - 1;
        off = 0x1000;

        if (start < 0) {
            off += start;
            start = 0;
        }
        if (end > 0xFFFFFF) {
            end = 0xFFFFFF;
        }
        data.resize(end - start + 1);

        for (int j = 0, i = start; i <= end; j++, i++) {
            data[j] = static_cast<char>(mem_peek_byte(static_cast<uint32_t>(i)));
        }

        edit->setBase(start);
        edit->setData(data);
        if (second) {
            edit->setCursorOffset(off * 2 + 1);
        } else {
            edit->setOffset(off);
        }
    } else {
        int start = edit->getBase();
        int len = edit->getSize();
        data.resize(len);

        for (int j = 0, i = start; j < len; j++, i++) {
            data[j] = static_cast<char>(mem_peek_byte(static_cast<uint32_t>(i)));
        }

        edit->setData(data);
    }
}

void MainWindow::flashGotoPressed() {
    if (GotoDialog dlg(m_flashGotoAddr, m_memGotoHistory, QStringList(), this); dlg.exec() == QDialog::Accepted) {
        const QString typed = dlg.text().trimmed();
        bool ok = false;
        const QString resolved = resolveAddressOrEquate(typed, &ok);
        if (ok) {
            m_flashGotoAddr = typed;
            if (!ui->flashEdit->getSize() && guiEmuValid) {
                ui->flashEdit->setDataSize(static_cast<int>(qMin(mem.flash.size, UINT32_C(0xC00000))));
            }
            ui->flashEdit->setFocus();
            ui->flashEdit->setOffset(hex2int(resolved));
            ui->flashEdit->refreshVisibleData();

            auto &hist = m_memGotoHistory;
            std::erase_if(hist, [&](const QString &s){ return s.compare(typed, Qt::CaseInsensitive) == 0; });
            hist.insert(hist.begin(), typed);
            if (hist.size() > 50) { hist.resize(50); }
        } else {
            QMessageBox::warning(this, MSG_WARNING, tr("Error when reading input string"));
        }
    }
}

void MainWindow::ramGotoPressed() {
    GotoDialog dlg(m_RamGotoAddr, m_memGotoHistory, QStringList(), this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString typed = dlg.text().trimmed();
        bool ok = false;
        const QString resolved = resolveAddressOrEquate(typed, &ok);
        if (ok) {
            m_RamGotoAddr = typed;
            if (!ui->ramEdit->getSize() && guiEmuValid) {
                ui->ramEdit->setBase(0xD00000);
                ui->ramEdit->setDataSize(0x65800);
            }
            ui->ramEdit->setFocus();
            ui->ramEdit->setOffset(hex2int(resolved) - 0xD00000);
            ui->ramEdit->refreshVisibleData();

            auto &hist = m_memGotoHistory;
            std::erase_if(hist, [&](const QString &s){ return s.compare(typed, Qt::CaseInsensitive) == 0; });
            hist.insert(hist.begin(), typed);
            if (hist.size() > 50) { hist.resize(50); }
        } else {
            QMessageBox::warning(this, MSG_WARNING, tr("Error when reading input string"));
        }
    }
}

void MainWindow::memSearchEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    SearchWidget search(m_searchStr, m_searchMode);
    int searchMode, found = 0;
    search.show();

    if ((searchMode = search.exec()) == SearchWidget::Cancel) {
        return;
    }

    m_searchMode = search.getType();
    m_searchStr = search.getSearchString();

    QString searchString;
    if (m_searchMode == SearchWidget::Hex) {
        searchString = m_searchStr;
    } else {
        searchString = QString::fromStdString(m_searchStr.toLatin1().toHex().toStdString());
    }

    edit->setFocus();
    std::string s = searchString.toUpper().toStdString();
    if (searchString.isEmpty() || (searchString.length() & 1) || s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
        QMessageBox::critical(this, MSG_ERROR, tr("Error when reading input string"));
        return;
    }

    QByteArray searchBa = QByteArray::fromHex(searchString.toLatin1());

    switch (searchMode) {
        default:
        case SearchWidget::NextNot:
            found = edit->indexNotOf(searchBa);
            break;
        case SearchWidget::Prev:
            found = edit->indexPrevOf(searchBa);
            break;
        case SearchWidget::PrevNot:
            found = edit->indexPrevNotOf(searchBa);
            break;
        case SearchWidget::Next:
            found = edit->indexOf(searchBa);
            break;
    }

    if (found == -1) {
         QMessageBox::warning(this, MSG_WARNING, tr("String not found."));
    }
}

void MainWindow::memGoto(HexWidget *edit, uint32_t address) {
    if (edit == Q_NULLPTR || !guiEmuValid) {
        return;
    }

    if (guiDebug) {
        edit->setBase(static_cast<int>(address));
        edit->setOffset(0);
        memUpdateEdit(edit, true);
        edit->setHighlight(static_cast<int>(address));
        return;
    }

    constexpr int windowSize = 0x1000;
    const uint32_t base = static_cast<uint32_t>(edit->getBase());
    const uint32_t size = static_cast<uint32_t>(edit->getSize());
    if (!size || address < base || address - base >= size) {
        constexpr uint32_t addressSpaceSize = UINT32_C(0x1000000);
        uint32_t start = address > windowSize / 2 ? address - windowSize / 2 : 0;
        if (start > addressSpaceSize - windowSize) {
            start = addressSpaceSize - windowSize;
        }
        edit->setBase(static_cast<int>(start));
        edit->setDataSize(windowSize);
    }
    edit->setOffset(static_cast<int>(address) - edit->getBase());
    edit->refreshVisibleData();
    edit->setHighlight(static_cast<int>(address));
}

void MainWindow::memGotoEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    GotoDialog dlg(m_memGotoAddr, m_memGotoHistory, QStringList(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QString typed = dlg.text().trimmed();
        bool ok = false;
        QString resolved = resolveAddressOrEquate(typed, &ok);
        if (ok) {
            m_memGotoAddr = typed;
            memGoto(edit, static_cast<uint32_t>(hex2int(resolved)));
            // MRU update
            auto &hist = m_memGotoHistory;
            std::erase_if(hist, [&](const QString &s){ return s.compare(typed, Qt::CaseInsensitive) == 0; });
            hist.insert(hist.begin(), typed);
            if (hist.size() > 50) { hist.resize(50); }
        } else {
            QMessageBox::warning(this, MSG_WARNING, tr("Error when reading input string"));
        }
    }
}

void MainWindow::memSync(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    debugPopulate();
    disasmUpdateAddr(m_disasmAddr, m_disasmPane);
    edit->setFocus();
}

void MainWindow::flashSyncPressed() {
    if (ui->flashEdit->modifiedCount()) {
        memcpy(mem.flash.block, ui->flashEdit->data(), qMin(mem.flash.size, (uint32_t)ui->flashEdit->getSize()));
    }
    memSync(ui->flashEdit);
}

void MainWindow::ramSyncPressed() {
    if (ui->ramEdit->modifiedCount()) {
        memcpy(mem.ram.block, ui->ramEdit->data(), 0x65800);
    }
    memSync(ui->ramEdit);
}

void MainWindow::memSyncEdit(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    int base = edit->getBase();
    int count = edit->modifiedCount();
    for (int i = 0; count && i < edit->getSize(); i++) {
        if (edit->modified()[i]) {
            mem_poke_byte(static_cast<uint32_t>(base + i), edit->data()[i]);
            count--;
        }
        qApp->processEvents();
    }

    memSync(edit);
}

void MainWindow::memAsciiToggle(HexWidget *edit) {
    if (edit == Q_NULLPTR) {
        return;
    }

    edit->setAsciiArea(!edit->getAsciiArea());
}

void MainWindow::contextMem(const QPoint &posa) {
    HexWidget *p = qobject_cast<HexWidget*>(sender());
    contextMemWidget(p->mapToGlobal(posa), static_cast<uint32_t>(p->getOffset() + p->getBase()));
}

void MainWindow::contextMemWidget(const QPoint &pos, uint32_t address) {
    QString addr = int2hex(address, 6);

    QMenu menu;
    QAction *copyAddr = menu.addAction(ACTION_COPY_ADDR + QStringLiteral(" '") + addr + QStringLiteral("'"));
    menu.addSeparator();
    QAction *toggleBreak = menu.addAction(ACTION_TOGGLE_BREAK);
    QAction *toggleRead = menu.addAction(ACTION_TOGGLE_READ);
    QAction *toggleWrite = menu.addAction(ACTION_TOGGLE_WRITE);
    QAction *toggleReadWrite = menu.addAction(ACTION_TOGGLE_RW);
    menu.addSeparator();
    QAction *gotoDisasm = gotoDisasmAction(&menu);

    QAction* item = menu.exec(pos);
    if (item == copyAddr) {
        qApp->clipboard()->setText(addr.toLatin1());
    } else if (item == gotoDisasm) {
        gotoDisasmAddr(address);
    } else if (item == toggleBreak) {
        breakAdd(breakNextLabel(), address, true, true, false);
        memDocksUpdate();
    } else if (item == toggleRead) {
        watchAdd(watchNextLabel(), address, address, DBG_MASK_READ, true, false);
        memDocksUpdate();
    } else if (item == toggleWrite) {
        watchAdd(watchNextLabel(), address, address, DBG_MASK_WRITE, true, false);
        memDocksUpdate();
    } else if (item == toggleReadWrite) {
        watchAdd(watchNextLabel(), address, address, DBG_MASK_READ | DBG_MASK_WRITE, true, false);
        memDocksUpdate();
    }
}
