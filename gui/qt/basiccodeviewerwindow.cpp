#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

#include "tivarslib/TypeHandlers/TypeHandlers.h"

#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>

BasicEditor::BasicEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    highlighter = new BasicHighlighter(document());

    lineNumberArea = new LineNumberArea(this);

    QFont font = QFont(QStringLiteral("TICELarge"), 11);
    lineNumberArea->setFont(font);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &BasicEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &BasicEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &BasicEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
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

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

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

void BasicEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void BasicEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    painter.setPen(Qt::black);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(-2, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

/* Highlighter */

BasicHighlighter::BasicHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    numberFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp(R"((((\b[0-9]+)?\.)?\b[0-9]+(ᴇ⁻?[0-9]+)?))");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    variableFormat.setForeground(Qt::darkYellow);
    rule.pattern = QRegExp(R"((\[[A-J]\])|([A-Zθ])|([\|?uvw])|((GDB|Str|Pic|Img)[0-9])|([XYr][₁₂₃₄₅₆₇₈₉]ᴛ?)|([XY](min|max|scl|res)))");
    rule.format = variableFormat;
    highlightingRules.append(rule);

    listFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp("(⌊[A-Zθ][A-Z0-9θ]{0,4})|(L[₁₂₃₄₅₆₇₈₉])");
    rule.format = listFormat;
    highlightingRules.append(rule);

    keywordFormat.setForeground(Qt::darkBlue);
    rule.pattern = QRegExp("\\b(Else|End|For|Goto|EndIf|ElseIf|End!If|If|!If|Lbl|Repeat|Return|Stop|Then|While)\\b");
    rule.format = keywordFormat;
    highlightingRules.append(rule);

    builtinFormat.setForeground(Qt::darkCyan);
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
        rule.pattern = QRegExp(pattern);
        rule.format = builtinFormat;
        highlightingRules.append(rule);
    }

    constFormat.setForeground(QColor("orange"));
    rule.pattern = QRegExp("(BLUE|RED|BLACK|MAGENTA|GREEN|ORANGE|BROWN|NAVY|LTBLUE|YELLOW|WHITE|LTGRAY|MEDGRAY|GRAY|DARKGRAY)");
    rule.format = constFormat;
    highlightingRules.append(rule);

    numberFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp(R"((((\b[0-9]+)?\.)?\b[0-9]+([eE][-+]?[0-9]+)?\b))");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    labelFormat.setForeground(QColor::fromRgb(0xBD, 0x3B, 0xB1));
    rule.pattern = QRegExp("\\b(Lbl|Goto) [A-Z0-9θ]{1,2}\\b");
    rule.format = labelFormat;
    highlightingRules.append(rule);

    prgmFormat.setForeground(QColor::fromRgb(0xCD, 0x5C, 0x5C));
    rule.pattern = QRegExp("prgm[A-Z][A-Z0-9θ]{0,7}\\b");
    rule.format = prgmFormat;
    highlightingRules.append(rule);

    delvarFormat.setForeground(Qt::darkCyan);
    rule.pattern = QRegExp("DelVar ");
    rule.format = delvarFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(QColor::fromRgb(0x1A, 0x96, 0x28));
    rule.pattern = QRegExp(R"("[^→"]*(→|"|$))");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    otherFormat.setForeground(Qt::black);
    rule.pattern = QRegExp("→");
    rule.format = otherFormat;
    highlightingRules.append(rule);
}

void BasicHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);
}

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *parent) : QDialog{parent}, ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);
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

void BasicCodeViewerWindow::setOriginalCode(const QString &code) {
    m_originalCode = code;
    m_formattedCode = QString::fromStdString(tivars::TH_Tokenized::reindentCodeString(m_originalCode.toStdString()));
    showCode();
}

void BasicCodeViewerWindow::toggleHighlight() {
    m_showingHighlighted ^= true;
    ui->basicEdit->toggleHighlight();
    showCode();
}

void BasicCodeViewerWindow::toggleWrap() {
    m_showingWrapped ^= true;
    ui->basicEdit->setWordWrapMode(m_showingWrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    showCode();
}

void BasicCodeViewerWindow::toggleFormat() {
    m_showingFormatted ^= true;
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
        ui->basicEdit->repaint();
    }
}

BasicCodeViewerWindow::~BasicCodeViewerWindow() {
    delete ui;
}
