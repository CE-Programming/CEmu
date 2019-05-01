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

    ui->checkBasicHighlighting->setChecked(m_basicShowingHighlighted);
    ui->checkBasicShowFetch->setChecked(m_basicShowingFetches);
    ui->checkBasicShowTempParse->setChecked(m_basicShowingTempParse);
    ui->checkBasicLiveExecution->setChecked(m_basicShowingLiveExecution);

    connect(ui->checkBasicHighlighting, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleHighlight);
    connect(ui->checkBasicShowFetch, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleShowFetch);
    connect(ui->checkBasicShowTempParse, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleShowTempParse);
    connect(ui->checkBasicLiveExecution, &QCheckBox::toggled, this, &MainWindow::debugBasicToggleLiveExecution);

    connect(ui->buttonBasicStep, &QPushButton::clicked, this, &MainWindow::debugBasicStep);
    connect(ui->buttonBasicStepNext, &QPushButton::clicked, this, &MainWindow::debugBasicStepNext);
    connect(ui->buttonBasicRun, &QPushButton::clicked, [this]{
        debug_set_mode(DBG_MODE_BASIC, m_basicShowingFetches);
        debugBasicToggle();
    });

    ui->basicEdit->setWordWrapMode(m_basicShowingFetches ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    ui->basicEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));
    ui->basicTempEdit->setWordWrapMode(m_basicShowingFetches ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    ui->basicTempEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));

    m_basicCurrToken.format.setBackground(QColor(Qt::yellow).lighter(100));
    m_basicCurrLine.format.setBackground(QColor(Qt::blue).lighter(180));
    m_basicCurrLine.format.setProperty(QTextFormat::FullWidthSelection, true);
}

MainWindow::debug_basic_status_t MainWindow::debugBasicRaise() {
    return debugBasicEnable();
}

MainWindow::debug_basic_status_t MainWindow::debugBasicEnable() {
    guiDebugBasic = true;
    return debugBasicGuiState(true);
}

void MainWindow::debugBasicDisable() {
    guiDebugBasic = false;
    debugBasicGuiState(false);
}

void MainWindow::debugBasicToggle() {
    bool state = guiDebugBasic;
    bool live = m_basicShowingLiveExecution;

    if (m_pathRom.isEmpty()) {
        return;
    }

    if (live) {
        debugBasicToggleLiveExecution();
    }

    if (state) {
        debugBasicDisable();
    }

    if (live) {
        debugBasicToggleLiveExecution();
    }

    emu.debug(!state, DBG_MODE_BASIC);
}

void MainWindow::debugBasicLeave(bool allowRefresh) {
    bool state = guiDebugBasic;
    bool live = m_basicShowingLiveExecution;

    if (m_pathRom.isEmpty()) {
        return;
    }

    if (allowRefresh && live) {
        debugBasicToggleLiveExecution();
    }

    if (state) {
        debugBasicDisable();
    }

    if (allowRefresh && live) {
        debugBasicToggleLiveExecution();
    }

    emu.debug(!state, DBG_MODE_BASIC);
}

MainWindow::debug_basic_status_t MainWindow::debugBasicGuiState(bool state) {
    if (state) {
        ui->buttonBasicRun->setText(tr("Run"));
        ui->buttonBasicRun->setIcon(m_iconRun);
    } else {
        ui->buttonBasicRun->setText(tr("Stop"));
        ui->buttonBasicRun->setIcon(m_iconStop);
    }

    ui->buttonBasicStep->setEnabled(state);
    ui->buttonBasicStepNext->setEnabled(state);

    m_basicPrgmsTokensMap.clear();
    m_basicPrgmsMap.clear();
    m_basicPrgmsOriginalCode.clear();
    m_basicPrgmsFormattedCode.clear();
    m_basicOriginalCode = Q_NULLPTR;
    m_basicFormattedCode = Q_NULLPTR;

    m_basicPrgmsTokensMap.push_back(QList<token_highlight_t>());
    m_basicPrgmsOriginalCode.push_back(QString());
    m_basicPrgmsFormattedCode.push_back(QString());

    if (state) {
        return debugBasicUpdate(true);
    }

    return DBG_BASIC_NO_EXECUTING_PRGM;
}

MainWindow::debug_basic_status_t MainWindow::debugBasicPgrmLookup(bool allowSwitch, int *idx) {
    const QString *origReference = m_basicOriginalCode;

    m_basicTempOpen = false;

    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        ui->labelBasicStatus->setText(tr("No Basic Program Executing."));
        ui->basicEdit->clear();
        ui->basicTempEdit->clear();
        return DBG_BASIC_NO_EXECUTING_PRGM;
    } else {
        QString var_name = QString(calc_var_name_to_utf8(reinterpret_cast<uint8_t*>(&name[1])));
        QString map_name = QString(&name[1]);

        // lookup in map to see if we've already parsed this file
        QList<int> values = m_basicPrgmsMap.values(var_name);
        if (!values.isEmpty()) {
            int index = values.first();
            if (idx) {
                *idx = index;
            }
            if (origReference != m_basicPrgmsOriginalCode.at(index)) {
                m_basicOriginalCode = &m_basicPrgmsOriginalCode.at(index);
                m_basicFormattedCode = &m_basicPrgmsFormattedCode.at(index);
                return DBG_BASIC_NEED_REFRESH;
            }
            return DBG_BASIC_NO_REFRESH;
        } else {
            calc_var_type_t type = static_cast<calc_var_type_t>(name[0]);

            // find the program in memory
            const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
            const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

            const QByteArray prgmBytes(reinterpret_cast<const char*>(phys_mem_ptr(static_cast<uint32_t>(begPC), 3)), endPC - begPC + 1);
            QString str;

            try {
                str = QString::fromStdString(tivars::TIVarType::createFromID(CALC_VAR_TYPE_PROG).getHandlers().second(data_t(prgmBytes.constData(), prgmBytes.constEnd()), options_t({ {"fromRawBytes", true} })));
            } catch(...) {
                return DBG_BASIC_NO_EXECUTING_PRGM;
            }

            if (type == CALC_VAR_TYPE_TEMP_PROG ||
                type == CALC_VAR_TYPE_EQU ||
                name[1] == '$') {

                if (!m_basicShowingTempParse) {
                    return DBG_BASIC_NO_EXECUTING_PRGM;
                }

                debugBasicCreateTokenMap(0, prgmBytes, begPC);

                if (allowSwitch) {
                    ui->tabDebugBasic->setCurrentIndex(1);
                }
                m_basicTempOpen = true;
                m_basicOriginalCodeTemp = str;
                m_basicFormattedCodeTemp = QString::fromStdString(tivars::TH_Tokenized::reindentCodeString(str.toStdString()));
                m_basicOriginalCode = &m_basicOriginalCodeTemp;
                m_basicFormattedCode = &m_basicFormattedCodeTemp;
            } else {
                if (allowSwitch) {
                    ui->tabDebugBasic->setCurrentIndex(0);
                }

                int index = m_basicPrgmsOriginalCode.count();
                if (idx) {
                    *idx = index;
                }

                m_basicPrgmsTokensMap.push_back(QList<token_highlight_t>());
                debugBasicCreateTokenMap(index, prgmBytes, begPC);

                m_basicPrgmsMap[var_name] = index;
                m_basicPrgmsOriginalCode.append(str);
                m_basicPrgmsFormattedCode.append(str);
                m_basicOriginalCode = &m_basicPrgmsOriginalCode.last();
                m_basicFormattedCode = &m_basicPrgmsFormattedCode.last();
            }
        }
        if (m_basicTempOpen == false) {
            ui->labelBasicStatus->setText(tr("Executing Program: ") + var_name);
        }
    }
    return DBG_BASIC_NEED_REFRESH;
}

/* function to parse the program and store the mapping of all bytes to highlights */
/* who cares about eating all of the user's ram */
void MainWindow::debugBasicCreateTokenMap(int idx, const QByteArray &data, int base) {
    token_highlight_t posinfo = { 0, 0, 0 };

    for (int i = 0; i < data.size();) {
        uint8_t token = static_cast<uint8_t>(data[i]);
        uint8_t tokenNext = i < data.size() - 1 ? static_cast<uint8_t>(data[i + 1]) : static_cast<uint8_t>(-1u);

        // check for newline
        if (token == 0x3F) {
            posinfo.len = 1;
            m_basicPrgmsTokensMap[idx].append(posinfo); // need new lines for temp parser
            posinfo.line++;
            posinfo.offset++;
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

        if (incr == 2) {
            m_basicPrgmsTokensMap[idx].append(posinfo);
            m_basicPrgmsTokensMap[idx].append(posinfo);
        } else {
            m_basicPrgmsTokensMap[idx].append(posinfo);
        }

        if (!tokStr.empty()) {
            posinfo.offset += posinfo.len;
        }
        i += incr;
    }
}

void MainWindow::debugBasicStep() {
    debug_step(DBG_BASIC_STEP, 0u);
    debugBasicLeave(false);
}

void MainWindow::debugBasicStepNext() {
    // locate next line
    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

    int watchPC = begPC;
/*
    int curLine = m_basicPrgmsTokensMap[curPC].line;

    for (int i = curPC; i < endPC; i++) {
        if (m_basicPrgmsTokensMap[i].line == curLine + 1) {
            watchPC = i;
            break;
        }
    }
*/
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

MainWindow::debug_basic_status_t MainWindow::debugBasicUpdate(bool force) {
    static int prevCurPC;

    if (!force && !m_basicShowingLiveExecution) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

    if (curPC > endPC || curPC < begPC || prevCurPC == curPC) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    prevCurPC = curPC;

    int index = 0;
    debug_basic_status_t status = debugBasicPgrmLookup(force, &index);
    switch (status) {
        case DBG_BASIC_NEED_REFRESH:
            if (m_basicOriginalCode != Q_NULLPTR) {
                if (m_basicTempOpen) {
                    ui->basicTempEdit->document()->setPlainText(*m_basicOriginalCode);
                } else {
                    ui->basicEdit->document()->setPlainText(*m_basicOriginalCode);
                }
            }
            break;
        case DBG_BASIC_NO_REFRESH:
            break;
        default:
            return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    // quick lookup using lists rather than map/hash
    const token_highlight_t posinfo = m_basicPrgmsTokensMap[index][curPC - begPC];

    if (m_basicTempOpen == false) {
        m_basicCurrToken.cursor = QTextCursor(ui->basicEdit->document());
    } else {
        m_basicCurrToken.cursor = QTextCursor(ui->basicTempEdit->document());
    }

    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Start, QTextCursor::MoveMode::MoveAnchor, 0);
    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, posinfo.offset);
    m_basicCurrLine.cursor = m_basicCurrToken.cursor;
    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, posinfo.len);

    if (m_basicTempOpen == false) {
        ui->basicEdit->setTextCursor(m_basicCurrLine.cursor);
        ui->basicEdit->setExtraSelections({ m_basicCurrLine, m_basicCurrToken });
    } else {
        ui->basicTempEdit->setTextCursor(m_basicCurrLine.cursor);
        ui->basicTempEdit->setExtraSelections({ m_basicCurrLine, m_basicCurrToken });
    }

    return DBG_BASIC_SUCCESS;
}

void MainWindow::debugBasicToggleHighlight() {
    m_basicShowingHighlighted = !m_basicShowingHighlighted;
    ui->basicEdit->toggleHighlight();
    ui->basicTempEdit->toggleHighlight();
    debugBasicShowCode();
}

void MainWindow::debugBasicToggleShowFetch() {
    m_basicShowingFetches = !m_basicShowingFetches;
    debug_set_mode(DBG_MODE_BASIC, m_basicShowingFetches);
    debugBasicShowCode();
}

void MainWindow::debugBasicToggleShowTempParse() {
    m_basicShowingTempParse = !m_basicShowingTempParse;
    debugBasicShowCode();
}

void MainWindow::debugBasicToggleLiveExecution() {
    m_basicShowingLiveExecution = !m_basicShowingLiveExecution;
    debugBasicShowCode();
}

void MainWindow::debugBasicShowCode() {
    if (m_basicOriginalCode != Q_NULLPTR) {
        if (m_basicTempOpen) {
            ui->basicTempEdit->document()->setPlainText(*m_basicOriginalCode);
        } else {
            ui->basicEdit->document()->setPlainText(*m_basicOriginalCode);
        }
    }
}

