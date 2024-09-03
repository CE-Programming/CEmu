#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

#include "utils.h"

#include "tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h"
#include "tivars_lib_cpp/src/tivarslib_utils.h"

#include "../../core/mem.h"

#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMessageBox>

BasicEditor::BasicEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    highlighter = new BasicHighlighter(document());
    lineNumberArea = new LineNumberArea(this);

    QFont font = QFont(QStringLiteral("TICELarge"), 11);
    lineNumberArea->setFont(font);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &BasicEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &BasicEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);
}

void BasicEditor::updateDarkMode()
{
    if (highlighter != nullptr) {
        delete highlighter;
        highlighter = new BasicHighlighter(document());
    }
}

void BasicEditor::toggleHighlight()
{
    if (highlighter == nullptr) {
        highlighter = new BasicHighlighter(document());
    } else {
        delete highlighter;
        highlighter = nullptr;
    }
}

int BasicEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int singlespace;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    singlespace = fontMetrics().horizontalAdvance(QLatin1Char('9'));
#else
    singlespace = fontMetrics().width(QLatin1Char('9'));
#endif

    int space = 3 + singlespace * digits;

    return space;
}

void BasicEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void BasicEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void BasicEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void BasicEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    painter.setPen(Qt::black);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(-2, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

/* Highlighter */

BasicHighlighter::BasicHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    bool darkMode = isRunningInDarkMode();

    numberFormat.setForeground(QColor(darkMode ? "lime" : "darkmagenta"));
    rule.pattern = QRegularExpression(R"((((\b[0-9]+)?\.)?\b[0-9]+(ᴇ⁻?[0-9]+)?))");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    variableFormat.setForeground(QColor(darkMode ? "yellow" : "darkyellow"));
    rule.pattern = QRegularExpression(R"((\[[A-J]\])|([A-Zθ])|([\|?uvw])|((GDB|Str|Pic|Img)[0-9])|([XYr][₁₂₃₄₅₆₇₈₉]ᴛ?)|([XY](min|max|scl|res)))");
    rule.format = variableFormat;
    highlightingRules.append(rule);

    listFormat.setForeground(QColor(darkMode ? "lightblue" : "blue"));
    rule.pattern = QRegularExpression("(⌊[A-Zθ][A-Z0-9θ]{0,4})|(L[₁₂₃₄₅₆₇₈₉])");
    rule.format = listFormat;
    highlightingRules.append(rule);

    keywordFormat.setForeground(QColor(darkMode ? "darkorange" : "darkblue"));
    rule.pattern = QRegularExpression("\\b(Else|End|For|Goto|EndIf|ElseIf|End!If|If|!If|Lbl|Repeat|Return|Stop|Then|While)\\b");
    rule.format = keywordFormat;
    highlightingRules.append(rule);

    builtinFormat.setForeground(darkMode ? Qt::cyan : Qt::darkCyan);
    QStringList builtinPatterns;
    builtinPatterns << "\\bAns\\b" << "\\bAsmComp\\b" << "\\bAsm(8[34]CE?)?Prgm\\b" <<"\\bAUTO\\b"
                    << "\\bAxesOff\\b" << "\\bAxesOn\\b" << "\\bBackgroundOn\\b" << "\\bBackgroundOff\\b"
                    << "\\bCLASSIC\\b" << "\\bClockOff\\b" << "\\bClockOn\\b" << "\\bConnected\\b" << "\\bCoordOff\\b"
                    << "\\bCoordOn\\b" << "\\bDiagnosticOff\\b" << "\\bDiagnosticOn\\b" << "\\bDot\\b" << "\\beval\\b"
                    << "\\bExprOff\\b" << "\\bExprOn\\b" << "\\bFnOff\\b" << "\\bFnOn\\b" << "\\bGridOff\\b"
                    << "\\bGridOn\\b" << "\\bHistogram\\b" << "\\bHoriz\\b" << "\\bLabelOff\\b" << "\\bLabelOn\\b"
                    << "\\bMATHPRINT\\b" << "\\bNormal\\b" << "\\bParam\\b" << "\\bPlot1\\b" << "\\bPlot2\\b"
                    << "\\bPlot3\\b" << "\\bPlotsOff\\b" << "\\bPlotsOn\\b" << "\\bPolar\\b" << "\\bPolarGC\\b"
                    << "\\bprgm\\b" << "\\bReal\\b" << "\\bSetUpEditor\\b" << "\\bWeb\\b" << "\\bZ-Test\\b"
                    << "\\bZBox\\b" << "\\bZDecimal\\b" << "\\bZFrac\\b" << "\\bZInteger\\b" << "\\bZInterval\\b"
                    << "\\bZoom\\b" << "\\bZoomFit\\b" << "\\bZoomRcl\\b" << "\\bZoomStat\\b" << "\\bZoomSto\\b"
                    << "\\bZPrevious\\b" << "\\bZQuadrant1\\b" << "\\bZSquare\\b" << "\\bZStandard\\b" << "\\bZTrig\\b"
                    << "\\babs\\b" << "\\bangle\\b" << "\\bANOVA\\b" << "\\bAnswer\\b" << "\\bArchive\\b" << "\\bAsm\\b"
                    << "\\baugment\\b" << "\\bbal\\b" << "\\bbinomcdf\\b" << "\\bbinompdf\\b" << "\\bBoxplot\\b" << "\\bChange\\b"
                    << "\\bcheckTmr\\b" << "\\bCircle\\b" << "\\bClear\\b" << "\\bClrAllLists\\b" << "\\bClrDraw\\b"
                    << "\\bClrHome\\b" << "\\bEffÉcran\\b" << "\\bClrList\\b" << "\\bClrTable\\b" << "\\bconj\\b"
                    << "\\bcos\\b" << "\\bcosh\\b" << "\\bCubicReg\\b" << "\\bcumSum\\b" << "\\bdayOfWk\\b" << "\\bdbd\\b"
                    << "\\bDec\\b" << "\\bDEC\\b" << "\\bDegree\\b" << "\\bDelVar\\b" << "\\bDependAsk\\b"
                    << "\\bDependAuto\\b" << "\\bdet\\b" << "\\bdim\\b" << "\\bDisp\\b" << "\\bDispGraph\\b"
                    << "\\bDispTable\\b" << "\\bDMS\\b" << "\\bDrawF\\b" << "\\bDrawInv\\b" << "\\bDS\\b" << "\\bEff\\b"
                    << "\\bEng\\b" << "\\bEntries\\b" << "\\bEqu\\b" << "\\bExecLib\\b" << "\\bexpr\\b" << "\\bExpReg\\b"
                    << "\\bFcdf\\b" << "\\bFill\\b" << "\\bFix\\b" << "\\bFloat\\b" << "\\bfMax\\b" << "\\bfMin\\b"
                    << "\\bfnInt\\b" << "\\bfPart\\b" << "\\bFpdf\\b" << "\\bFrac\\b" << "\\bFRAC\\b" << "\\bFull\\b"
                    << "\\bFunc\\b" << "\\bGarbageCollect\\b" << "\\bgcd\\b" << "\\bgeometcdf\\b" << "\\bgeometpdf\\b"
                    << "\\bGet\\b" << "\\bGetCalc\\b" << "\\bgetDate\\b" << "\\bgetDtFmt\\b" << "\\bgetDtStr\\b"
                    << "\\bgetKey\\b" << "\\bgetTime\\b" << "\\bgetTmFmt\\b" << "\\bgetTmStr\\b" << "\\bGraphStyle\\b"
                    << "\\bHorizontal\\b" << "\\bidentity\\b" << "\\bimag\\b" << "\\bIn\\b" << "\\bIndpntAsk\\b"
                    << "\\bIndpntAuto\\b" << "\\bInput\\b" << "\\binString\\b" << "\\bint\\b" << "\\binvNorm\\b"
                    << "\\binvT\\b" << "\\biPart\\b" << "\\birr\\b" << "\\bIS\\b" << "\\bIsClockOn\\b" << "\\blcm\\b"
                    << "\\blength\\b" << "\\bLine\\b" << "\\bLinReg\\b" << "\\bLinRegTInt\\b" << "\\bLinRegTTest\\b"
                    << "\\blist\\b" << "\\bList\\b" << "\\bln\\b" << "\\bLnReg\\b" << "\\blog\\b" << "\\blogBASE\\b"
                    << "\\bLogistic\\b" << "\\bManual\\b" << "\\bFit\\b" << "\\bmatr\\b" << "\\bMatr\\b" << "\\bmax\\b"
                    << "\\bmean\\b" << "\\bMed-Med\\b" << "\\bmedian\\b" << "\\bMenu\\b" << "\\bmin\\b" << "\\bModBoxplot\\b"
                    << "\\bnCr\\b" << "\\bnDeriv\\b" << "\\bNom\\b" << "\\bnormalcdf\\b" << "\\bnormalpdf\\b"
                    << "\\bNormProbPlot\\b" << "\\bnot\\b" << "\\bnPr\\b" << "\\bnpv\\b" << "\\bOff\\b" << "\\bOn\\b"
                    << "\\bOpenLib\\b" << "\\bor\\b" << "\\bOut\\b" << "\\bOutput\\b" << "\\bPause\\b" << "\\bPmt_Bgn\\b"
                    << "\\bPmt_End\\b" << "\\bpoissoncdf\\b" << "\\bpoissonpdf\\b" << "\\bprod\\b" << "\\bPrompt\\b"
                    << "\\bPropZInt\\b" << "\\bPropZTest\\b" << "\\bPt\\b" << "\\bPwrReg\\b" << "\\bPxl\\b" << "\\bPxl-Test\\b"
                    << "\\bPxl-Change\\b" << "\\bQuadReg\\b" << "\\bQuartReg\\b" << "\\bRadian\\b" << "\\brand\\b"
                    << "\\brandBin\\b" << "\\brandInt\\b" << "\\bnbrAléatEnt\\b" << "\\brandIntNoRep\\b" << "\\brandM\\b"
                    << "\\brandNorm\\b" << "\\breal\\b" << "\\bRecallGDB\\b" << "\\bRecallPic\\b" << "\\bRect\\b"
                    << "\\bRectGC\\b" << "\\bref\\b" << "\\bremainder\\b" << "\\bround\\b" << "\\brow\\b" << "\\browSwap\\b"
                    << "\\brref\\b" << "\\bSampFTest\\b" << "\\bSampTInt\\b" << "\\bSampTTest\\b" << "\\bSampZInt\\b"
                    << "\\bSampZTest\\b" << "\\bScatter\\b" << "\\bSci\\b" << "\\bSelect\\b" << "\\bSend\\b" << "\\bSeq\\b"
                    << "\\bSequential\\b" << "\\bsetDate\\b" << "\\bsetDtFmt\\b" << "\\bsetTime\\b" << "\\bsetTmFmt\\b"
                    << "\\bShade\\b" << "\\bShade_t\\b" << "\\bShadeF\\b" << "\\bShadeNorm\\b" << "\\bSimul\\b" << "\\bsin\\b"
                    << "\\bsinh\\b" << "\\bsinh\\b" << "\\bSinReg\\b" << "\\bsolve\\b" << "\\bSortA\\b" << "\\bSortD\\b"
                    << "\\bstartTmr\\b" << "\\bStats\\b" << "\\bstatwizard\\b" << "\\bstdDev\\b" << "\\bStoreGDB\\b"
                    << "\\bStorePic\\b" << "\\bString\\b" << "\\bsub\\b" << "\\bsum\\b" << "\\btan\\b" << "\\bTangent\\b"
                    << "\\btanh\\b" << "\\btcdf\\b" << "\\bTest\\b" << "\\bText\\b" << "\\bTextColor\\b" << "\\bTime\\b"
                    << "\\btimeCnv\\b" << "\\bTInterval\\b" << "\\btoString\\b" << "\\btpdf\\b" << "\\bTrace\\b"
                    << "\\btvm_FV\\b" << "\\btvm_I\\b" << "\\btvm_N\\b" << "\\btvm_Pmt\\b" << "\\btvm_PV\\b" << "\\bUn\\b"
                    << "\\bUnArchive\\b" << "\\buvAxes\\b" << "\\buwAxes\\b" << "\\bVar\\b" << "\\bvariance\\b"
                    << "\\bVertical\\b" << "\\bvwAxes\\b" << "\\bWait\\b" << "\\bxor\\b" << "\\bxyLine\\b";
    foreach (const QString &pattern, builtinPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = builtinFormat;
        highlightingRules.append(rule);
    }

    constFormat.setForeground(QColor("orange"));
    rule.pattern = QRegularExpression("(BLUE|RED|BLACK|MAGENTA|GREEN|ORANGE|BROWN|NAVY|LTBLUE|YELLOW|WHITE|LTGRAY|MEDGRAY|GRAY|DARKGRAY)");
    rule.format = constFormat;
    highlightingRules.append(rule);

    labelFormat.setForeground(darkMode ? QColor(Qt::magenta) : QColor::fromRgb(0xBD, 0x3B, 0xB1));
    rule.pattern = QRegularExpression("\\b(Lbl|Goto) [A-Z0-9θ]{1,2}\\b");
    rule.format = labelFormat;
    highlightingRules.append(rule);

    prgmFormat.setForeground(QColor::fromRgb(0xCD, 0x5C, 0x5C));
    rule.pattern = QRegularExpression("prgm[A-Z][A-Z0-9θ]{0,7}\\b");
    rule.format = prgmFormat;
    highlightingRules.append(rule);

    delvarFormat.setForeground(darkMode ? Qt::cyan : Qt::darkCyan);
    rule.pattern = QRegularExpression("DelVar ");
    rule.format = delvarFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(QColor::fromRgb(0x1A, 0x96, 0x28));
    rule.pattern = QRegularExpression(R"("[^→"]*(→|"|$))");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    otherFormat.setForeground(darkMode ? Qt::lightGray : Qt::black);
    rule.pattern = QRegularExpression("→");
    rule.format = otherFormat;
    highlightingRules.append(rule);
}

void BasicHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        const QRegularExpression expression(rule.pattern);
        QRegularExpressionMatchIterator iter = expression.globalMatch(text);
        while (iter.hasNext()) {
            const auto& match = iter.next();
            if (match.hasMatch()) {
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
    setCurrentBlockState(0);
}

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *parent, bool doHighlight, bool doWrap, bool doFormat)
  : QDialog{parent}, ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);

    // Considering the default values...
    if (!doHighlight) { m_showingHighlighted = false; ui->checkboxHighlighting->setCheckState(Qt::Unchecked); ui->basicEdit->toggleHighlight(); }
    if (doWrap) { m_showingWrapped = true; ui->checkboxLineWrapping->setCheckState(Qt::Checked); }
    if (doFormat) { m_showingFormatted = true; ui->checkboxReformatting->setCheckState(Qt::Checked); }

    connect(ui->checkboxHighlighting, &QCheckBox::toggled, this, &BasicCodeViewerWindow::toggleHighlight);
    connect(ui->checkboxLineWrapping, &QCheckBox::toggled, this, &BasicCodeViewerWindow::toggleWrap);
    connect(ui->checkboxReformatting, &QCheckBox::toggled, this, &BasicCodeViewerWindow::toggleFormat);

    ui->basicEdit->setWordWrapMode(m_showingWrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);

    // Add special jacobly font
    ui->basicEdit->setFont(QFont(QStringLiteral("TICELarge"), 11));
}

void BasicCodeViewerWindow::setVariableName(const QString &name) {
    m_variableName = name;
    setWindowTitle(tr("Variable viewer") + QStringLiteral(" | ") + m_variableName);
}

void BasicCodeViewerWindow::setOriginalCode(const QString &code, bool reindent) {
    m_originalCode = code;
    if (reindent) {
        m_formattedCode = QString::fromStdString(tivars::TypeHandlers::TH_Tokenized::reindentCodeString(m_originalCode.toStdString()));
    } else {
        m_formattedCode = m_originalCode;
    }
    showCode();
}

void BasicCodeViewerWindow::toggleHighlight() {
    m_showingHighlighted = !m_showingHighlighted;
    ui->basicEdit->toggleHighlight();
    showCode();
}

void BasicCodeViewerWindow::toggleWrap() {
    m_showingWrapped = !m_showingWrapped;
    ui->basicEdit->setWordWrapMode(m_showingWrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    showCode();
}

void BasicCodeViewerWindow::toggleFormat() {
    m_showingFormatted = !m_showingFormatted;
    const int scrollValue = ui->basicEdit->verticalScrollBar()->value();
    ui->basicEdit->document()->setPlainText(m_showingFormatted ? m_formattedCode : m_originalCode);
    ui->basicEdit->verticalScrollBar()->setValue(scrollValue);
    showCode();
}

void BasicCodeViewerWindow::showCode() {
    if (!hasCodeYet) {
        ui->basicEdit->document()->setPlainText(m_showingFormatted ? m_formattedCode : m_originalCode);
        hasCodeYet = true;
    } else {
        ui->basicEdit->update();
    }
}

BasicCodeViewerWindow::~BasicCodeViewerWindow() {
    delete ui;
}

void LineNumberArea::anchor() {} /* weak vtable thing */
