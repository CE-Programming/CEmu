#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dockwidget.h"
#include "utils.h"
#include "visualizerwidget.h"

#include "tivars_lib_cpp/src/TIVarType.h"
#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"

#include "../../core/mem.h"

#include <QtWidgets/QScrollBar>

void MainWindow::debugBasicInit() {
    debugBasicDisable();

    debug.basicLiveExecution = m_basicShowingLiveExecution;


    ui->checkBasicHighlighting->setChecked(m_basicShowingHighlighted);
    ui->checkBasicLineWrapping->setChecked(m_basicShowingWrapped);
    ui->checkBasicReformatting->setChecked(m_basicShowingFormatted);
    ui->checkBasicLiveExecution->setChecked(m_basicShowingLiveExecution);

    connect(ui->checkBasicHighlighting, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleHighlight);
    connect(ui->checkBasicLineWrapping, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleWrap);
    connect(ui->checkBasicReformatting, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleFormat);
    connect(ui->checkBasicLiveExecution, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleLiveExecution);

    connect(ui->buttonBasicStep, &QPushButton::clicked, this, &MainWindow::debugBasicStep);
    connect(ui->buttonBasicRun, &QPushButton::clicked, [this]{ debug_set_mode(DBG_MODE_BASIC); debugBasicToggle(); });

    ui->basicEdit->setWordWrapMode(m_basicShowingWrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    ui->basicEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));
}

void MainWindow::debugBasicRaise() {
    debugBasicEnable();
}

void MainWindow::debugBasicEnable() {
    guiDebugBasic = true;
    debugBasicGuiState(true);
}

void MainWindow::debugBasicDisable() {
    guiDebugBasic = false;
    debugBasicGuiState(false);
}

void MainWindow::debugBasicToggle() {
    bool state = guiDebugBasic;

    if (m_pathRom.isEmpty()) {
        return;
    }

    if (state) {
        debugBasicDisable();
    }

    emu.debug(!state, DBG_MODE_BASIC);
}

void MainWindow::debugBasicGuiState(bool state) {
    if (state) {
        ui->buttonBasicRun->setText(tr("Run"));
        ui->buttonBasicRun->setIcon(m_iconRun);
    } else {
        ui->buttonBasicRun->setText(tr("Stop"));
        ui->buttonBasicRun->setIcon(m_iconStop);
    }

    ui->buttonBasicStep->setEnabled(state);

    // refresh program view
    if (state) {
        m_basicPrgmsMap.clear();
        m_basicPrgmsOriginalCode.clear();
        m_basicPrgmsFormattedCode.clear();
        debugBasicLiveUpdate();
    } else {
        m_basicPrgmsMap.clear();
        m_basicPrgmsOriginalCode.clear();
        m_basicPrgmsFormattedCode.clear();
    }
}

int MainWindow::debugBasicPgrmLookup() {
    const QString *origReference = m_basicOriginalCode;
    int refresh = 0;

    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        ui->labelBasicStatus->setText(tr("No Basic Program Executing."));
        ui->basicEdit->clear();
    } else {
        calc_var_t var;
        calc_var_t search;
        QString var_name = QString(&name[1]);

        // lookup in map to see if we've already parsed this file
        QList<int> values = m_basicPrgmsMap.values(var_name);
        if (!values.isEmpty()) {
            int index = values.first();
            if (origReference != m_basicPrgmsOriginalCode.at(index)) {
                m_basicOriginalCode = &m_basicPrgmsOriginalCode.at(index);
                m_basicFormattedCode = &m_basicPrgmsFormattedCode.at(index);
                refresh = 1;
            }
        } else {
            memcpy(search.name, &name[1], 8);
            search.namelen = static_cast<uint8_t>(var_name.length());
            search.type = static_cast<calc_var_type>(name[0]);

            if (search.type == CALC_VAR_TYPE_TEMP_PROG ||
                search.type == CALC_VAR_TYPE_EQU ||
                name[1] == '$') {
                return 2;
            }

            // find the program in memory
            if (vat_search_find(&search, &var)) {
                QString str;

                try {
                    str = QString::fromStdString(tivars::TIVarType::createFromID(CALC_VAR_TYPE_PROG).getHandlers().second(data_t(var.data, var.data + var.size), options_t()));
                } catch(...) {
                    return 0;
                }

                m_basicPrgmsMap[var_name] = m_basicPrgmsOriginalCode.count();
                m_basicPrgmsOriginalCode.append(str);
                m_basicPrgmsFormattedCode.append(QString::fromStdString(tivars::TH_Tokenized::reindentCodeString(str.toStdString())));
                m_basicOriginalCode = &m_basicPrgmsOriginalCode.last();
                m_basicFormattedCode = &m_basicPrgmsFormattedCode.last();
                refresh = 1;
            }
        }
        ui->labelBasicStatus->setText(tr("Executing Program: ") + var_name);
    }
    return refresh;
}

void MainWindow::debugBasicStep() {
    debug_step(DBG_BASIC_STEP, 0u);
    debugBasicToggle();
}

int MainWindow::debugBasicLiveUpdate() {
    if (!m_basicShowingLiveExecution) {
        return 0;
    }

    int refresh = debugBasicPgrmLookup();
    switch (refresh) {
        default:
            break;
        case 1:
            ui->basicEdit->document()->setPlainText(m_basicShowingFormatted ? *m_basicFormattedCode : *m_basicOriginalCode);
            break;
        case 2:
            return 2;
    }

    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

    if (curPC >= endPC || curPC < begPC) {
        return 2;
    }

    const QByteArray tmp(reinterpret_cast<const char*>(phys_mem_ptr(static_cast<uint32_t>(begPC), 3)), curPC - begPC + 1);
    const data_t prgmPartialBytes(tmp.constData(), tmp.constEnd());

    const auto posinfo = tivars::TH_Tokenized::getPosInfoAtOffset(prgmPartialBytes, static_cast<uint16_t>(curPC - begPC), options_t());

    QTextEdit::ExtraSelection currToken;
    currToken.format.setBackground(QColor(Qt::yellow).lighter(100));
    currToken.cursor = QTextCursor(ui->basicEdit->document());
    currToken.cursor.clearSelection();
    currToken.cursor.movePosition(QTextCursor::MoveOperation::Down, QTextCursor::MoveMode::MoveAnchor, posinfo.line);
    currToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, posinfo.column);
    currToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, posinfo.len);

    QTextEdit::ExtraSelection currLine;
    currLine.format.setBackground(QColor(Qt::blue).lighter(180));
    currLine.format.setProperty(QTextFormat::FullWidthSelection, true);
    currLine.cursor = QTextCursor(ui->basicEdit->document());
    currLine.cursor.movePosition(QTextCursor::MoveOperation::Down, QTextCursor::MoveMode::MoveAnchor, posinfo.line);
    currLine.cursor.clearSelection();

    ui->basicEdit->setExtraSelections({ currLine, currToken });

    return 1;
}

void MainWindow::debugBasicToggleHighlight() {
    m_basicShowingHighlighted = !m_basicShowingHighlighted;
    ui->basicEdit->toggleHighlight();
    debugBasicShowCode();
}

void MainWindow::debugBasicToggleWrap() {
    m_basicShowingWrapped = !m_basicShowingWrapped;
    ui->basicEdit->setWordWrapMode(m_basicShowingWrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    debugBasicShowCode();
}

void MainWindow::debugBasicToggleFormat() {
    m_basicShowingFormatted = !m_basicShowingFormatted;
    const int scrollValue = ui->basicEdit->verticalScrollBar()->value();
    ui->basicEdit->document()->setPlainText(m_basicShowingFormatted ? *m_basicFormattedCode : *m_basicOriginalCode);
    ui->basicEdit->verticalScrollBar()->setValue(scrollValue);
    ui->basicEdit->update();
}

void MainWindow::debugBasicToggleLiveExecution() {
    m_basicShowingLiveExecution = !m_basicShowingLiveExecution;
    debug.basicLiveExecution = m_basicShowingLiveExecution;
}

void MainWindow::debugBasicShowCode() {
    ui->basicEdit->document()->setPlainText(m_basicShowingFormatted ? *m_basicFormattedCode : *m_basicOriginalCode);
    ui->basicEdit->update();
}
