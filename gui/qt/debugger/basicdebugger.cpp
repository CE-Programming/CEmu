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
    connect(ui->buttonBasicStepNext, &QPushButton::clicked, this, &MainWindow::debugBasicStepNext);
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
    ui->buttonBasicStepNext->setEnabled(state);

    // refresh program view
    if (state) {
        m_basicPrgmsTokensMap.clear();
        m_basicPrgmsMap.clear();
        m_basicPrgmsOriginalCode.clear();
        m_basicPrgmsFormattedCode.clear();
        debugBasicLiveUpdate();
    } else {
        m_basicPrgmsTokensMap.clear();
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
            calc_var_type_t type = static_cast<calc_var_type_t>(name[0]);

            if (type == CALC_VAR_TYPE_TEMP_PROG ||
                type == CALC_VAR_TYPE_EQU ||
                name[1] == '$') {
                return 2;
            }

            // find the program in memory
            const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
            const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

            if (begPC > endPC) {
                return 2;
            }

            const QByteArray prgmBytes(reinterpret_cast<const char*>(phys_mem_ptr(static_cast<uint32_t>(begPC), 3)), endPC - begPC + 1);
            QString str;

            try {
                str = QString::fromStdString(tivars::TIVarType::createFromID(CALC_VAR_TYPE_PROG).getHandlers().second(data_t(prgmBytes.constData(), prgmBytes.constEnd()), options_t({ {"fromRawBytes", true} })));
            } catch(...) {
                return 0;
            }

            debugBasicCreateTokenMap(prgmBytes, begPC);

            m_basicPrgmsMap[var_name] = m_basicPrgmsOriginalCode.count();
            m_basicPrgmsOriginalCode.append(str);
            m_basicPrgmsFormattedCode.append(QString::fromStdString(tivars::TH_Tokenized::reindentCodeString(str.toStdString())));
            m_basicOriginalCode = &m_basicPrgmsOriginalCode.last();
            m_basicFormattedCode = &m_basicPrgmsFormattedCode.last();
            refresh = 1;
        }
        ui->labelBasicStatus->setText(tr("Executing Program: ") + var_name);
    }
    return refresh;
}

/* function to parse the program and store the mapping of all bytes to highlights */
/* who cares about eating all of the user's ram */
void MainWindow::debugBasicCreateTokenMap(const QByteArray &data, int base) {
    token_highlight_t posinfo = { 0, 0, 0 };

    for (int i = 0; i < data.size();) {
        uint8_t token = static_cast<uint8_t>(data[i]);
        uint8_t tokenNext = i < data.size() - 1 ? static_cast<uint8_t>(data[i + 1]) : static_cast<uint8_t>(-1u);

        // check for newline
        if (token == 0x3F) {
            m_basicPrgmsTokensMap[base + i] = posinfo;
            posinfo.line++;
            posinfo.column = 0;
            posinfo.len = 0;
            i++;
            continue;
        }

        // get current token
        int incr;
        data_t tokBytes(2);
        tokBytes[0] = token;
        tokBytes[1] = tokenNext;
        std::string tokStr = tivars::TH_Tokenized::tokenToString(tokBytes, &incr, options_t({ {"prettify", true} }));

        if (!tokStr.empty()) {
            posinfo.len = utf8_strlen(tokStr);
        }

        m_basicPrgmsTokensMap[base + i] = posinfo;
        if (incr == 2) {
            m_basicPrgmsTokensMap[base + i + 1] = posinfo;
        }

        if (!tokStr.empty()) {
            posinfo.column += posinfo.len;
        }
        i += incr;
    }
}

void MainWindow::debugBasicStep() {
    debug_step(DBG_BASIC_STEP, 0u);
    debugBasicToggle();
}

void MainWindow::debugBasicStepNext() {
    // locate next line
    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));

    int curLine = m_basicPrgmsTokensMap[curPC].line;
    int watchPC = begPC;

    for (int i = curPC; i < endPC; i++) {
        if (m_basicPrgmsTokensMap[i].line == curLine + 1) {
            watchPC = i;
            break;
        }
    }

    debug_step(DBG_BASIC_STEP_NEXT, static_cast<uint32_t>(watchPC));
    debugBasicToggle();
}

QString MainWindow::debugBasicGetPrgmName() {
    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        return QString();
    } else {
        return QString(&name[1]);
    }
}

int MainWindow::debugBasicLiveUpdate() {
    static int prevCurPC;

    if (!m_basicShowingLiveExecution) {
        return 0;
    }

    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

    if (curPC >= endPC || curPC < begPC || prevCurPC == curPC) {
        return 2;
    }

    prevCurPC = curPC;

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

    const QByteArray tmp(reinterpret_cast<const char*>(phys_mem_ptr(static_cast<uint32_t>(begPC), 3)), curPC - begPC + 1);
    const data_t prgmPartialBytes(tmp.constData(), tmp.constEnd());

    if (!m_basicPrgmsTokensMap.contains(curPC)) {
        return 2;
    }
    const token_highlight_t posinfo = m_basicPrgmsTokensMap[curPC];

    QTextEdit::ExtraSelection currToken;
    currToken.format.setBackground(QColor(Qt::yellow).lighter(100));
    currToken.cursor = QTextCursor(ui->basicEdit->document());
    currToken.cursor.clearSelection();
    currToken.cursor.movePosition(QTextCursor::MoveOperation::NextBlock, QTextCursor::MoveMode::MoveAnchor, posinfo.line);
    currToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, posinfo.column);
    currToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, posinfo.len);

    QTextEdit::ExtraSelection currLine;
    currLine.format.setBackground(QColor(Qt::blue).lighter(180));
    currLine.format.setProperty(QTextFormat::FullWidthSelection, true);
    currLine.cursor = QTextCursor(ui->basicEdit->document());
    currLine.cursor.movePosition(QTextCursor::MoveOperation::NextBlock, QTextCursor::MoveMode::MoveAnchor, posinfo.line);
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
