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
    debugBasicClearCache();
    debugBasic(false);

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
    connect(ui->btnDebugBasicRun, &QPushButton::clicked, this, &MainWindow::debugToggle);

    ui->basicEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));
    ui->basicTempEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));

    m_basicCurrLine.format.setProperty(QTextFormat::FullWidthSelection, true);
    debugBasicUpdateDarkMode();
}

void MainWindow::debugBasic(bool enable) {
    guiDebugBasic = enable;
    ui->tabDebugBasic->setEnabled(enable);
    ui->btnDebugBasicEnable->setChecked(enable);
    debugBasicGuiState(guiDebug);
    debugBasicReconfigure(true);
}

void MainWindow::debugBasicUpdateDarkMode() {
    bool darkMode = isRunningInDarkMode();
    m_basicCurrLine.format.setBackground(darkMode ? QColor(Qt::darkBlue) : QColor(Qt::blue).lighter(180));
    m_basicCurrToken.format.setBackground(darkMode ? QColor(Qt::darkMagenta) : QColor(Qt::yellow).lighter(100));
    for (BasicEditor *basicEditor : { ui->basicEdit, ui->basicTempEdit }) {
        basicEditor->updateDarkMode();
        if (!basicEditor->extraSelections().isEmpty()) {
            ui->basicEdit->setExtraSelections({ m_basicCurrLine, m_basicCurrToken });
        }
    }
}

void MainWindow::debugBasicReconfigure(bool forceUpdate) {
    if (guiDebug) {
        if (guiDebugBasic) {
            debug_enable_basic_mode(m_basicShowFetches, m_basicShowLiveExecution);
            debugBasicUpdate(forceUpdate);
            if (!forceUpdate && !m_basicShowLiveExecution) {
                debugBasicClearHighlights();
            }
        } else {
            debug_disable_basic_mode();
            watchUpdate();
            debugBasicClearCache();
            debugBasicClearEdits();
        }
    } else {
        emu.debug(true, EmuThread::RequestBasicDebugger);
    }
}

void MainWindow::debugBasicRaise() {
    debugRaise();
}

void MainWindow::debugBasicClearCache() {
    m_basicPrgmsTokensMap.clear();
    m_basicPrgmsMap.clear();
    m_basicPrgmsOriginalBytes.clear();
    m_basicPrgmsOriginalCode.clear();
    //m_basicPrgmsFormattedCode.clear();
    m_basicCodeIndex = 0;
    m_basicTempParserNeedsRefresh = false;

    m_basicPrgmsTokensMap.push_back(QList<token_highlight_t>());
    m_basicPrgmsOriginalBytes.push_back(QByteArray());
    m_basicPrgmsOriginalCode.push_back(QString());
    //m_basicPrgmsFormattedCode.push_back(QString());

    if (!m_basicShowTempParser) {
        ui->basicTempEdit->clear();
    }

    m_basicClearCache = false;
}

void MainWindow::debugBasicClearEdits() {
    ui->basicEdit->clear();
    ui->basicTempEdit->clear();
    ui->labelBasicStatus->setText(tr("No Basic Program Executing."));
    m_basicCodeIndex = 0;
}

void MainWindow::debugBasicClearHighlights() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    ui->basicEdit->setExtraSelections(extraSelections);
    ui->basicTempEdit->setExtraSelections(extraSelections);
}

void MainWindow::debugBasicGuiState(bool state) {
    if (state) {
        ui->btnDebugBasicRun->setText(tr("Run"));
        ui->btnDebugBasicRun->setIcon(m_iconRun);
    } else {
        ui->btnDebugBasicRun->setText(tr("Stop"));
        ui->btnDebugBasicRun->setIcon(m_iconStop);
    }

    ui->btnDebugBasicStep->setEnabled(state && guiDebugBasic);
    ui->btnDebugBasicStepNext->setEnabled(state && guiDebugBasic);
}

MainWindow::debug_basic_status_t MainWindow::debugBasicPrgmLookup(bool allowSwitch, int *idx) {
    if (m_basicClearCache) {
        debugBasicClearCache();
    }

    char name[10];
    if (!debug_get_executing_basic_prgm(name)) {
        debugBasicClearEdits();
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    // find the program in memory
    const uint32_t begPC = mem_peek_long(DBG_BASIC_BEGPC);
    const uint32_t endPC = mem_peek_long(DBG_BASIC_ENDPC);
    if (endPC < begPC) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    uint32_t prgmSize = endPC - begPC + 1;
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
    if (prgmSize == (uint32_t)m_basicPrgmsOriginalBytes[index].size() &&
        !memcmp(m_basicPrgmsOriginalBytes[index].constData(), prgmBytesPtr, prgmSize)) {
        // check if the currently displayed program was switched
        if ((index == 0 && !m_basicTempParserNeedsRefresh) || index == m_basicCodeIndex) {
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
    if (status == DBG_BASIC_NEED_REFRESH) {
        if (index != 0) {
            ui->labelBasicStatus->setText(tr("Executing Program: ") + var_name);
        } else {
            m_basicTempParserNeedsRefresh = true;
        }
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
    debugBasicStepInternal(false);
}

void MainWindow::debugBasicStepNext() {
    debugBasicStepInternal(true);
}

void MainWindow::debugBasicStepInternal(bool next) {
    if (!guiDebug || !guiDebugBasic) {
        return;
    }

    const uint32_t begPC = mem_peek_long(DBG_BASIC_BEGPC);
    const uint32_t curPC = mem_peek_long(DBG_BASIC_CURPC);
    const uint32_t endPC = mem_peek_long(DBG_BASIC_ENDPC);
    int index = 0;
    if (curPC < begPC || curPC > endPC || debugBasicPrgmLookup(false, &index) == DBG_BASIC_NO_EXECUTING_PRGM) {
        // step until valid
        const uint16_t firstOffset = 1;
        const uint16_t lastOffset = 0;
        debugSync();
        debug_step(DBG_BASIC_STEP_IN, firstOffset | (static_cast<uint32_t>(lastOffset) << 16));
        emu.resume();
        return;
    }

    // locate next line
    uint32_t offset = curPC - begPC;
    const auto &tokensMap = m_basicPrgmsTokensMap[index];
    const auto stepField = m_basicShowFetches ? &token_highlight_t::offset : &token_highlight_t::line;
    const int currField = tokensMap[offset].*stepField;
    const auto compareFields = [=](const token_highlight_t &posInfo) {
        return (posInfo.*stepField == currField) ^ next;
    };
    const auto first = std::find_if(tokensMap.constBegin() + offset, tokensMap.constEnd(), compareFields);
    const auto last = std::find_if_not(first, tokensMap.constEnd(), compareFields);
    if (first != last) {
        const uint16_t firstOffset = (first - tokensMap.constBegin());
        const uint16_t lastOffset = (last - tokensMap.constBegin()) - 1;
        debugSync();
        debug_step(next ? DBG_BASIC_STEP_NEXT : DBG_BASIC_STEP_IN, firstOffset | (static_cast<uint32_t>(lastOffset) << 16));
        emu.resume();
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
    int index = 0;
    debug_basic_status_t status = debugBasicPrgmLookup(force, &index);
    if (status == DBG_BASIC_NO_EXECUTING_PRGM) {
        return DBG_BASIC_NO_EXECUTING_PRGM;
    }

    const uint32_t begPC = mem_peek_long(DBG_BASIC_BEGPC);
    const uint32_t curPC = mem_peek_long(DBG_BASIC_CURPC);
    const uint32_t endPC = mem_peek_long(DBG_BASIC_ENDPC);
    bool validPC = curPC >= begPC && curPC <= endPC;

    if (force || m_basicShowLiveExecution) {
        // show variables only if this is a live update, they get shown already for normal debug stop
        if (!force && guiReceive) {
            varShow();
        }

        if (status == DBG_BASIC_NEED_REFRESH && index != 0) {
            ui->basicEdit->document()->setPlainText(m_basicPrgmsOriginalCode[index]);
            m_basicCodeIndex = index;
        }
        if (m_basicTempParserNeedsRefresh) {
            ui->basicTempEdit->document()->setPlainText(m_basicPrgmsOriginalCode[0]);
            m_basicTempParserNeedsRefresh = false;
        }

        if (validPC) {
            // quick lookup using lists rather than map/hash
            const token_highlight_t posinfo = m_basicPrgmsTokensMap[index][curPC - begPC];

            BasicEditor *thisEditor = ui->basicEdit, *otherEditor = ui->basicTempEdit;
            if (index == 0) {
                std::swap(thisEditor, otherEditor);
            }
            m_basicCurrToken.cursor = QTextCursor(thisEditor->document());
            m_basicCurrToken.cursor.setPosition(posinfo.offset);
            m_basicCurrLine.cursor = m_basicCurrToken.cursor;
            m_basicCurrToken.cursor.movePosition(QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::KeepAnchor, posinfo.len);

            thisEditor->setTextCursor(m_basicCurrLine.cursor);
            thisEditor->setExtraSelections({ m_basicCurrLine, m_basicCurrToken });
            otherEditor->setExtraSelections({});
        } else {
            debugBasicClearHighlights();
        }
    }

    return validPC ? DBG_BASIC_SUCCESS : DBG_BASIC_NO_EXECUTING_PRGM;
}

void MainWindow::debugBasicToggleHighlight(bool enabled) {
    m_basicShowHighlighted = enabled;
    ui->basicEdit->toggleHighlight();
    ui->basicTempEdit->toggleHighlight();
}

void MainWindow::debugBasicToggleShowFetch(bool enabled) {
    m_basicClearCache = true;
    m_basicShowFetches = enabled;
    if (guiDebugBasic) {
        debugBasicReconfigure(true);
    }
}

void MainWindow::debugBasicToggleShowTempParse(bool enabled) {
    m_basicClearCache = true;
    m_basicShowTempParser = enabled;
    if (guiDebugBasic) {
        debugBasicReconfigure(true);
    }
}

void MainWindow::debugBasicToggleLiveExecution(bool enabled) {
    m_basicShowLiveExecution = enabled;
    if (guiDebugBasic) {
        debugBasicReconfigure(true);
    }
}


