#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dockwidget.h"
#include "utils.h"
#include "visualizerwidget.h"

#include "tivars_lib_cpp/src/TIVarType.h"
#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"

#include "../../core/cpu.h"
#include "../../core/mem.h"

#include <QtWidgets/QScrollBar>

void MainWindow::debugBasicInit() {
    debugBasicClearCache();
    debugBasic(false);
    debugBasicDisable();

    ui->btnDebugBasicHighlight->setChecked(m_basicShowHighlighted);
    ui->btnDebugBasicShowFetches->setChecked(m_basicShowFetches);
    ui->btnDebugBasicShowTempParser->setChecked(m_basicShowTempParser);
    ui->btnDebugBasicLiveExecution->setChecked(m_basicShowLiveExecution);

    connect(ui->btnDebugBasicHighlight, &QToolButton::toggled, this, &MainWindow::debugBasicToggleHighlight);
    connect(ui->btnDebugBasicShowFetches, &QToolButton::toggled, this, &MainWindow::debugBasicToggleShowFetch);
    connect(ui->btnDebugBasicShowTempParser, &QToolButton::toggled, this, &MainWindow::debugBasicToggleShowTempParse);
    connect(ui->btnDebugBasicLiveExecution, &QToolButton::toggled, this, &MainWindow::debugBasicToggleLiveExecution);
    connect(ui->btnDebugBasicEnable, &QToolButton::toggled, this, &MainWindow::debugBasic);

    connect(ui->btnDebugBasicStep, &QPushButton::clicked, this, &MainWindow::debugBasicStep);
    connect(ui->btnDebugBasicStepNext, &QPushButton::clicked, this, &MainWindow::debugBasicStepNext);
    connect(ui->btnDebugBasicRun, &QPushButton::clicked, this, &MainWindow::debugBasicToggle);

    ui->basicEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));
    ui->basicTempEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));

    m_basicCurrToken.format.setBackground(QColor(Qt::yellow).lighter(100));
    m_basicCurrLine.format.setBackground(QColor(Qt::blue).lighter(180));
    m_basicCurrLine.format.setProperty(QTextFormat::FullWidthSelection, true);
}

void MainWindow::debugBasic(bool enable) {
    ui->tabDebugBasic->setEnabled(enable);
    ui->btnDebugBasicRun->setEnabled(enable);
    ui->btnDebugBasicEnable->setChecked(enable);
    debug.basicMode = enable;
    if (enable) {
        debug_enable_basic_mode(m_basicShowFetches);
    } else {
        debugBasicClearEdits();
        debug_disable_basic_mode();
    }
    if (guiDebugBasic == true) {
        debugBasicToggle();
    }

    // disable other debugger / var list
    ui->buttonSend->setEnabled(!enable);
    ui->buttonResendFiles->setEnabled(!enable);
    ui->buttonRun->setEnabled(!enable);
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

    if (guiDebug || guiSend) {
        return;
    }

    if (m_pathRom.isEmpty()) {
        return;
    }

    debug_disable_basic_mode();

    if (state) {
        debugBasicDisable();
    }

    debug_enable_basic_mode(m_basicShowFetches);

    emu.debug(!state, EmuThread::RequestBasicDebugger);
}

void MainWindow::debugBasicLeave(bool allowRefresh) {
    bool state = guiDebugBasic;
    bool live = m_basicShowLiveExecution;

    if (m_pathRom.isEmpty()) {
        return;
    }

    if (allowRefresh && live) {
        debugBasicToggleLiveExecution(!live);
    }

    if (state) {
        debugBasicDisable();
    }

    if (allowRefresh && live) {
        debugBasicToggleLiveExecution(live);
    }

    emu.debug(!state, EmuThread::RequestBasicDebugger);
}

void MainWindow::debugBasicClearCache() {
    debug_enable_basic_mode(m_basicShowFetches);

    m_basicPrgmsTokensMap.clear();
    m_basicPrgmsMap.clear();
    m_basicPrgmsOriginalBytes.clear();
    m_basicPrgmsOriginalCode.clear();
    //m_basicPrgmsFormattedCode.clear();
    m_basicCodeIndex = 0;

    m_basicPrgmsTokensMap.push_back(QList<token_highlight_t>());
    m_basicPrgmsOriginalBytes.push_back(QByteArray());
    m_basicPrgmsOriginalCode.push_back(QString());
    //m_basicPrgmsFormattedCode.push_back(QString());

    m_basicClearCache = false;
}

void MainWindow::debugBasicClearEdits() {
    ui->basicEdit->clear();
    ui->basicTempEdit->clear();
    m_basicCodeIndex = 0;
}

MainWindow::debug_basic_status_t MainWindow::debugBasicGuiState(bool state) {
    if (state) {
        ui->btnDebugBasicRun->setText(tr("Run"));
        ui->btnDebugBasicRun->setIcon(m_iconRun);
    } else {
        ui->btnDebugBasicRun->setText(tr("Stop"));
        ui->btnDebugBasicRun->setIcon(m_iconStop);
    }

    ui->btnDebugBasicStep->setEnabled(state);
    ui->btnDebugBasicStepNext->setEnabled(state);

    //m_basicClearCache = true;

    if (state) {
        return debugBasicUpdate(true);
    } else if (!m_basicShowLiveExecution) {
        debugBasicClearEdits();
    }

    return DBG_BASIC_NO_EXECUTING_PRGM;
}

MainWindow::debug_basic_status_t MainWindow::debugBasicPrgmLookup(bool allowSwitch, int *idx) {
    if (m_basicClearCache == true) {
        debugBasicClearCache();
    }

    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        ui->labelBasicStatus->setText(tr("No Basic Program Executing."));
        debugBasicClearEdits();
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    // find the program in memory
    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));
    if (endPC < begPC) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    int prgmSize = endPC - begPC + 1;
    const char *prgmBytesPtr = static_cast<const char *>(phys_mem_ptr(static_cast<uint32_t>(begPC), prgmSize));
    if (!prgmBytesPtr) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    calc_var_type_t type = static_cast<calc_var_type_t>(name[0]);
    QString var_name = QString(calc_var_name_to_utf8(reinterpret_cast<uint8_t*>(&name[1]), strlen(&name[1]), true));

    int index = 0;

    if (type == CALC_VAR_TYPE_TEMP_PROG ||
        type == CALC_VAR_TYPE_EQU ||
        name[1] == '$') {

        if (!m_basicShowTempParser) {
            return DBG_BASIC_NO_EXECUTING_PRGM;
        }
    } else {
        // lookup in map to see if we've already parsed this file
        auto basicPrgmIter = m_basicPrgmsMap.constFind(var_name);
        if (basicPrgmIter != m_basicPrgmsMap.constEnd()) {
            index = *basicPrgmIter;
        } else {
            index = m_basicPrgmsOriginalBytes.count();
            m_basicPrgmsMap[var_name] = index;
            m_basicPrgmsOriginalBytes.push_back(QByteArray());
            m_basicPrgmsOriginalCode.push_back(QString());
            //m_basicPrgmsFormattedCode.push_back(QString());
            m_basicPrgmsTokensMap.push_back(QList<token_highlight_t>());
        }
    }

    debug_basic_status_t status = DBG_BASIC_NEED_REFRESH;
    // check if the original program data matches
    if (prgmSize == m_basicPrgmsOriginalBytes[index].size() &&
        !memcmp(m_basicPrgmsOriginalBytes[index].constData(), prgmBytesPtr, prgmSize)) {
        // check if the currently displayed program was switched
        if (index == 0 || index == m_basicCodeIndex) {
            status = DBG_BASIC_NO_REFRESH;
        }
    } else {
        QByteArray prgmBytes(prgmBytesPtr, prgmSize);
        QString str;
        try {
            const options_t detok_opts = { { "fromRawBytes", true }, { "prettify", true } };
            str = QString::fromStdString(tivars::TypeHandlers::TH_Tokenized::makeStringFromData(data_t(prgmBytes.constData(), prgmBytes.constEnd()), detok_opts));
        } catch (...) {
            return DBG_BASIC_NO_EXECUTING_PRGM;
        }

        debugBasicCreateTokenMap(index, prgmBytes);
        m_basicPrgmsOriginalBytes[index] = std::move(prgmBytes);
        //m_basicPrgmsFormattedCode[index] = QString::fromStdString(tivars::TypeHandlers::TH_Tokenized::reindentCodeString(str.toStdString()));
        m_basicPrgmsOriginalCode[index] = std::move(str);
    }

    if (idx) {
        *idx = index;
    }
    if (allowSwitch) {
        ui->tabDebugBasic->setCurrentIndex(index == 0);
    }
    if (index != 0 && status == DBG_BASIC_NEED_REFRESH) {
        ui->labelBasicStatus->setText(tr("Executing Program: ") + var_name);
    }
    return status;
}

// function to parse the program and store the mapping of all bytes to highlights
// who cares about eating all of the user's ram
void MainWindow::debugBasicCreateTokenMap(int idx, const QByteArray &data) {
    auto &tokensMap = m_basicPrgmsTokensMap[idx];
    tokensMap.clear();
    token_highlight_t posinfo = { 0, 0, 0 };
    int i = 0;

    // if we are doing normal debug, we just highlight based on the
    // entire string from (:,\n) to the next break
    if (!m_basicShowFetches) {
        while (i < data.size()) {
            bool instr = false;
            int j = 0;
            while (i < data.size() && data[i] != 0x3F) {
                uint8_t token = static_cast<uint8_t>(data[i]);
                uint8_t tokenNext = i < data.size() - 1 ? static_cast<uint8_t>(data[i + 1]) : static_cast<uint8_t>(-1u);

                // check for : (make sure not in string)
                if (token == 0x04) {
                    instr = false;
                }
                if (token == 0x2A) {
                    instr = !instr;
                } else if (token == 0x3E && !instr) {
                    break;
                }

                // get current token
                int incr;
                data_t tokBytes(2);
                tokBytes[0] = token;
                tokBytes[1] = tokenNext;
                std::string tokStr = tivars::TypeHandlers::TH_Tokenized::tokenToString(tokBytes, &incr, { { "prettify", true } });

                if (!tokStr.empty()) {
                    posinfo.len += utf8_strlen(tokStr.c_str());
                }

                i += incr;
                j += incr;
            }
            j++;
            while (j) {
                tokensMap.append(posinfo);
                j--;
            }
            posinfo.offset += posinfo.len + 1;
            posinfo.line++;
            posinfo.len = 0;
            i++;
        }
    } else {
        while (i < data.size()) {
            uint8_t token = static_cast<uint8_t>(data[i]);
            uint8_t tokenNext = i < data.size() - 1 ? static_cast<uint8_t>(data[i + 1]) : static_cast<uint8_t>(-1u);

            // check for newline
            if (token == 0x3F) {
                posinfo.len = 1;
                tokensMap.append(posinfo); // need new lines for temp parser
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
            std::string tokStr = tivars::TypeHandlers::TH_Tokenized::tokenToString(tokBytes, &incr, { {"prettify", true } });

            posinfo.len = tokStr.empty() ? 0 : utf8_strlen(tokStr.c_str());

            if (incr == 2) {
                tokensMap.append(posinfo);
                tokensMap.append(posinfo);
            } else {
                tokensMap.append(posinfo);
            }

            posinfo.offset += posinfo.len;
            i += incr;
        }
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

    if (curPC < begPC || curPC > endPC) {
        return;
    }

    int index = 0;
    if (debugBasicPrgmLookup(false, &index) == DBG_BASIC_NO_EXECUTING_PRGM) {
        return;
    }

    int offset = curPC - begPC;
    auto &tokensMap = m_basicPrgmsTokensMap[index];
    int curLine = tokensMap[offset].line;
    auto first = std::find_if(tokensMap.constBegin() + offset, tokensMap.constEnd(), [=](const token_highlight_t &posInfo) { return posInfo.line != curLine; });
    auto last = std::find_if(first, tokensMap.constEnd(), [=](const token_highlight_t &posInfo) { return posInfo.line != curLine + 1; });
    uint16_t firstOffset = first - tokensMap.constBegin();
    uint16_t lastOffset = last - tokensMap.constBegin();
    if (firstOffset < lastOffset) {
        debug_step(DBG_BASIC_STEP_NEXT, firstOffset | (static_cast<uint32_t>(lastOffset - 1) << 16));
        debugBasicLeave(false);
    }
}

QString MainWindow::debugBasicGetPrgmName() {
    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        return QString();
    } else {
        return QString(calc_var_name_to_utf8(reinterpret_cast<uint8_t*>(&name[1]), strlen(&name[1]), true));
    }
}

MainWindow::debug_basic_status_t MainWindow::debugBasicUpdate(bool force) {
    static int prevCurPC;
    static int prevCpuPC;
    static token_highlight_t prevPosinfo = { 0, 0, 0 };

    if (!force && !m_basicShowLiveExecution) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    const int begPC = static_cast<int>(mem_peek_long(DBG_BASIC_BEGPC));
    const int curPC = static_cast<int>(mem_peek_long(DBG_BASIC_CURPC));
    const int endPC = static_cast<int>(mem_peek_long(DBG_BASIC_ENDPC));

    if (curPC > endPC || curPC < begPC) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    int index = 0;
    debug_basic_status_t status = debugBasicPrgmLookup(force, &index);
    if (status == DBG_BASIC_NO_EXECUTING_PRGM) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    // quick lookup using lists rather than map/hash
    const token_highlight_t posinfo = m_basicPrgmsTokensMap[index][curPC - begPC];

    // skip already highlighted lines, but allow self-looping
    if (!m_basicShowFetches &&
        (prevCurPC != curPC || prevCpuPC != cpu.registers.pc.hl) &&
        prevPosinfo.len == posinfo.len &&
        prevPosinfo.offset == posinfo.offset &&
        prevPosinfo.line == posinfo.line) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    prevCurPC = curPC;
    prevCpuPC = cpu.registers.pc.hl;
    prevPosinfo = posinfo;

    if (guiReceive) {
        varShow();
    }

    BasicEditor *basicEditor = (index != 0) ? ui->basicEdit : ui->basicTempEdit;

    if (status == DBG_BASIC_NEED_REFRESH) {
        basicEditor->document()->setPlainText(m_basicPrgmsOriginalCode[index]);
        if (index != 0) {
            m_basicCodeIndex = index;
        }
    }

    m_basicCurrToken.cursor = QTextCursor(basicEditor->document());

    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Start, QTextCursor::MoveMode::MoveAnchor, 0);
    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, posinfo.offset);
    m_basicCurrLine.cursor = m_basicCurrToken.cursor;
    m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, posinfo.len);

    basicEditor->setTextCursor(m_basicCurrLine.cursor);
    basicEditor->setExtraSelections({ m_basicCurrLine, m_basicCurrToken });

    return DBG_BASIC_SUCCESS;
}

void MainWindow::debugBasicToggleHighlight(bool enabled) {
    m_basicShowHighlighted = enabled;
    ui->basicEdit->toggleHighlight();
    ui->basicTempEdit->toggleHighlight();
}

void MainWindow::debugBasicToggleShowFetch(bool enabled) {
    m_basicClearCache = true;
    m_basicShowFetches = enabled;
}

void MainWindow::debugBasicToggleShowTempParse(bool enabled) {
    m_basicClearCache = true;
    m_basicShowTempParser = enabled;
}

void MainWindow::debugBasicToggleLiveExecution(bool enabled) {
    //m_basicClearCache = true;
    if (!enabled && !guiDebugBasic) {
        debugBasicClearEdits();
    }
    m_basicShowLiveExecution = enabled;
}


