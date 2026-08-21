#include "lcddebugwidget.h"

#include "../../core/lcd.h"
#include "../../core/panel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <functional>
#include <limits>

namespace {

QSpinBox *makeSpinBox(int minimum, int maximum, QWidget *parent, int step = 1) {
    auto *spin = new QSpinBox(parent);
    spin->setRange(minimum, maximum);
    spin->setSingleStep(step);
    spin->setKeyboardTracking(false);
    return spin;
}

QLabel *makeSummaryLabel(QWidget *parent) {
    auto *label = new QLabel(parent);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    return label;
}

QWidget *makeScrollPage(QWidget *content, QWidget *parent) {
    auto *scroll = new QScrollArea(parent);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    return scroll;
}

void appendPixelFormats(QComboBox *combo) {
    for (int value = 0; value < 8; value++) {
        QString name;
        switch (value) {
            case 3: name = LcdDebugWidget::tr("12-bit"); break;
            case 5: name = LcdDebugWidget::tr("16-bit"); break;
            case 6: name = LcdDebugWidget::tr("18-bit"); break;
            default: name = LcdDebugWidget::tr("Reserved (%1)").arg(value); break;
        }
        combo->addItem(name, value);
    }
}

std::array<uint8_t, 2> wordBytes(uint16_t value) {
    return { static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value) };
}

template <size_t N>
void writeCommand(uint8_t command, const std::array<uint8_t, N> &params) {
    const bool applied = panel_debug_write_command(command, params.data(), params.size());
    Q_ASSERT_X(applied, "LcdDebugWidget", "Panel command parameter size mismatch");
    (void)applied;
}

template <typename T>
void writeCommand(uint8_t command, const T &params) {
    const bool applied = panel_debug_write_command(command, reinterpret_cast<const uint8_t *>(&params), sizeof(params));
    Q_ASSERT_X(applied, "LcdDebugWidget", "Panel command parameter size mismatch");
    (void)applied;
}

QString panelModeText(uint8_t mode) {
    switch (mode) {
        case PANEL_DM_MCU: return LcdDebugWidget::tr("MCU");
        case PANEL_DM_RGB: return LcdDebugWidget::tr("RGB");
        case PANEL_DM_VSYNC: return LcdDebugWidget::tr("VSYNC");
        default: return LcdDebugWidget::tr("Reserved");
    }
}

QString lcdPhaseText(enum lcd_comp phase) {
    switch (phase) {
        case LCD_SYNC: return LcdDebugWidget::tr("Sync");
        case LCD_BACK_PORCH: return LcdDebugWidget::tr("Back porch");
        case LCD_ACTIVE_VIDEO: return LcdDebugWidget::tr("Active video");
        case LCD_FRONT_PORCH: return LcdDebugWidget::tr("Front porch");
        case LCD_LNBU: return LcdDebugWidget::tr("Line buffer underflow");
        default: return LcdDebugWidget::tr("Unknown");
    }
}

constexpr std::array<int, 16> gammaPointLevels = {
    0, 1, 2, 4, 6, 13, 20, 27, 36, 43, 50, 57, 59, 61, 62, 63
};

constexpr std::array<const char *, 16> gammaPointNames = {
    "V0", "V1", "V2", "V4", "V6", "V13", "V20", "V27",
    "V36", "V43", "V50", "V57", "V59", "V61", "V62", "V63"
};

int gammaParameterValue(const panel_gamma_t &params, int point) {
    switch (point) {
        case 0: return params.V0;
        case 1: return params.V1;
        case 2: return params.V2;
        case 3: return params.V4;
        case 4: return params.V6;
        case 5: return params.V13;
        case 6: return params.V20;
        case 7: return params.V27;
        case 8: return params.V36;
        case 9: return params.V43;
        case 10: return params.V50;
        case 11: return params.V57;
        case 12: return params.V59;
        case 13: return params.V61;
        case 14: return params.V62;
        case 15: return params.V63;
        default: return 0;
    }
}

int gammaParameterMaximum(int point) {
    static constexpr std::array<int, 16> maximums = {
        15, 63, 63, 31, 31, 15, 127, 7, 7, 127, 15, 31, 31, 63, 63, 15
    };
    return maximums[static_cast<size_t>(point)];
}

void setGammaParameterValue(panel_gamma_t &params, int point, int value) {
    switch (point) {
        case 0: params.V0 = value; break;
        case 1: params.V1 = value; break;
        case 2: params.V2 = value; break;
        case 3: params.V4 = value; break;
        case 4: params.V6 = value; break;
        case 5: params.V13 = value; break;
        case 6: params.V20 = value; break;
        case 7: params.V27 = value; break;
        case 8: params.V36 = value; break;
        case 9: params.V43 = value; break;
        case 10: params.V50 = value; break;
        case 11: params.V57 = value; break;
        case 12: params.V59 = value; break;
        case 13: params.V61 = value; break;
        case 14: params.V62 = value; break;
        case 15: params.V63 = value; break;
        default: break;
    }
}

} // namespace

class GammaCurveEditor : public QWidget {
public:
    explicit GammaCurveEditor(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(260);
        setMinimumWidth(440);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAccessibleName(LcdDebugWidget::tr("ST7789 gamma curve editor"));
        setAccessibleDescription(LcdDebugWidget::tr(
            "Drag a control point or use the arrow keys to edit the selected gamma register."));
    }

    QSize sizeHint() const override {
        return QSize(620, 300);
    }

    void setCurves(const panel_gamma_t &positive, const panel_gamma_t &negative) {
        m_curves[0] = positive;
        m_curves[1] = negative;
        m_selectedPoint = -1;
        m_hoverPoint = -1;
        update();
        notifySelectionChanged();
    }

    const panel_gamma_t &positiveCurve() const { return m_curves[0]; }
    const panel_gamma_t &negativeCurve() const { return m_curves[1]; }

    void setActiveCurve(int curve) {
        m_activeCurve = std::clamp(curve, 0, 1);
        m_selectedPoint = -1;
        m_hoverPoint = -1;
        update();
        notifySelectionChanged();
    }

    void setInterpolation(int curve, int j0, int j1) {
        panel_gamma_t &params = m_curves[std::clamp(curve, 0, 1)];
        params.J0 = std::clamp(j0, 0, 3);
        params.J1 = std::clamp(j1, 0, 3);
        update();
        if (changed) {
            changed();
        }
    }

    QString selectionText() const {
        if (m_selectedPoint < 0) {
            return LcdDebugWidget::tr("Drag a control point to edit it; use arrow keys for fine adjustments.");
        }
        float curve[64];
        panel_get_gamma_curve(curve, &m_curves[m_activeCurve]);
        const int level = gammaPointLevels[static_cast<size_t>(m_selectedPoint)];
        return LcdDebugWidget::tr("%1 curve · %2 · input %3 · voltage %4 · register %5/%6")
            .arg(m_activeCurve == 0 ? LcdDebugWidget::tr("Positive") : LcdDebugWidget::tr("Negative"))
            .arg(QString::fromLatin1(gammaPointNames[static_cast<size_t>(m_selectedPoint)]))
            .arg(level)
            .arg(curve[level], 0, 'f', 3)
            .arg(gammaParameterValue(m_curves[m_activeCurve], m_selectedPoint))
            .arg(gammaParameterMaximum(m_selectedPoint));
    }

    std::function<void()> changed;
    std::function<void()> selectionChanged;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRectF graph = graphRect();
        QColor background = palette().color(QPalette::Base);
        QColor foreground = palette().color(QPalette::Text);
        QColor grid = palette().color(QPalette::Mid);
        background.setAlpha(180);
        grid.setAlpha(90);

        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawRoundedRect(graph.adjusted(-8, -8, 8, 8), 6, 6);

        painter.setFont(QFont(painter.font().family(), painter.font().pointSize() - 1));
        for (int step = 0; step <= 4; step++) {
            const qreal fraction = step / 4.0;
            const qreal y = graph.bottom() - fraction * graph.height();
            painter.setPen(QPen(grid, 1));
            painter.drawLine(QPointF(graph.left(), y), QPointF(graph.right(), y));
            painter.setPen(foreground);
            painter.drawText(QRectF(0, y - 9, graph.left() - 8, 18),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(fraction, 'f', step == 0 || step == 4 ? 0 : 2));
        }
        static constexpr std::array<int, 5> inputTicks = { 0, 16, 32, 48, 63 };
        for (int input : inputTicks) {
            const qreal x = graph.left() + graph.width() * input / 63.0;
            painter.setPen(QPen(grid, 1));
            painter.drawLine(QPointF(x, graph.top()), QPointF(x, graph.bottom()));
            painter.setPen(foreground);
            painter.drawText(QRectF(x - 20, graph.bottom() + 7, 40, 18), Qt::AlignHCenter, QString::number(input));
        }

        painter.save();
        painter.translate(13, graph.center().y());
        painter.rotate(-90);
        painter.drawText(QRectF(-70, -10, 140, 20), Qt::AlignCenter, LcdDebugWidget::tr("Normalized voltage"));
        painter.restore();
        painter.drawText(QRectF(graph.left(), graph.bottom() + 23, graph.width(), 18),
                         Qt::AlignCenter, LcdDebugWidget::tr("Input level"));

        float curves[2][64];
        panel_get_gamma_curve(curves[0], &m_curves[0]);
        panel_get_gamma_curve(curves[1], &m_curves[1]);
        const QColor colors[2] = { QColor(41, 128, 255), QColor(255, 145, 35) };

        QPainterPath averagePath;
        for (int level = 0; level < 64; level++) {
            const QPointF point = curvePoint(graph, level, (curves[0][level] + curves[1][level]) * 0.5f);
            level ? averagePath.lineTo(point) : averagePath.moveTo(point);
        }
        QColor averageColor = foreground;
        averageColor.setAlpha(150);
        painter.setPen(QPen(averageColor, 1.5, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(averagePath);

        for (int pass = 0; pass < 2; pass++) {
            const int curve = pass == 0 ? 1 - m_activeCurve : m_activeCurve;
            QPainterPath path;
            for (int level = 0; level < 64; level++) {
                const QPointF point = curvePoint(graph, level, curves[curve][level]);
                level ? path.lineTo(point) : path.moveTo(point);
            }
            QColor color = colors[curve];
            if (curve != m_activeCurve) {
                color.setAlpha(150);
            }
            painter.setPen(QPen(color, curve == m_activeCurve ? 2.5 : 1.5));
            painter.drawPath(path);
        }

        const QColor activeColor = colors[m_activeCurve];
        for (int point = 0; point < static_cast<int>(gammaPointLevels.size()); point++) {
            const int level = gammaPointLevels[static_cast<size_t>(point)];
            const QPointF position = curvePoint(graph, level, curves[m_activeCurve][level]);
            const bool selected = point == m_selectedPoint;
            const bool hovered = point == m_hoverPoint;
            painter.setPen(QPen(background, 2));
            painter.setBrush(selected ? foreground : activeColor);
            painter.drawEllipse(position, selected ? 6.0 : hovered ? 5.5 : 4.5,
                                selected ? 6.0 : hovered ? 5.5 : 4.5);
            if (selected) {
                painter.setPen(QPen(activeColor, 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(position, 9, 9);
            }
        }

        const qreal legendY = 12;
        drawLegendItem(painter, 52, legendY, colors[0], foreground, LcdDebugWidget::tr("Positive"), false);
        drawLegendItem(painter, 145, legendY, colors[1], foreground, LcdDebugWidget::tr("Negative"), false);
        drawLegendItem(painter, 240, legendY, averageColor, foreground, LcdDebugWidget::tr("Average"), true);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        const int point = pointAt(event->pos());
        if (point >= 0) {
            m_selectedPoint = point;
            m_dragging = true;
            setFocus(Qt::MouseFocusReason);
            updatePointFromPosition(event->pos());
            notifySelectionChanged();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (m_dragging) {
            updatePointFromPosition(event->pos());
            return;
        }
        const int hover = pointAt(event->pos());
        if (hover != m_hoverPoint) {
            m_hoverPoint = hover;
            setCursor(hover >= 0 ? Qt::OpenHandCursor : Qt::ArrowCursor);
            update();
        }
        if (hover >= 0) {
            const int previous = m_selectedPoint;
            m_selectedPoint = hover;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QToolTip::showText(event->globalPosition().toPoint(), selectionText(), this);
#else
            QToolTip::showText(event->globalPos(), selectionText(), this);
#endif
            m_selectedPoint = previous;
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && m_dragging) {
            m_dragging = false;
            setCursor(Qt::OpenHandCursor);
        }
    }

    void leaveEvent(QEvent *) override {
        if (!m_dragging) {
            m_hoverPoint = -1;
            setCursor(Qt::ArrowCursor);
            update();
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (m_selectedPoint < 0) {
            QWidget::keyPressEvent(event);
            return;
        }
        int delta = 0;
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_Right:
                delta = 1;
                break;
            case Qt::Key_Down:
            case Qt::Key_Left:
                delta = -1;
                break;
            case Qt::Key_PageUp:
                delta = 4;
                break;
            case Qt::Key_PageDown:
                delta = -4;
                break;
            case Qt::Key_Home:
                setSelectedParameter(0);
                return;
            case Qt::Key_End:
                setSelectedParameter(gammaParameterMaximum(m_selectedPoint));
                return;
            default:
                QWidget::keyPressEvent(event);
                return;
        }
        setSelectedParameter(gammaParameterValue(m_curves[m_activeCurve], m_selectedPoint) + delta);
    }

private:
    QRectF graphRect() const {
        return QRectF(rect()).adjusted(52, 30, -18, -46);
    }

    static QPointF curvePoint(const QRectF &graph, int level, float value) {
        return QPointF(graph.left() + graph.width() * level / 63.0,
                       graph.bottom() - graph.height() * value);
    }

    int pointAt(const QPointF &position) const {
        float curve[64];
        panel_get_gamma_curve(curve, &m_curves[m_activeCurve]);
        const QRectF graph = graphRect();
        int nearest = -1;
        qreal nearestDistance = 11.0;
        for (int point = 0; point < static_cast<int>(gammaPointLevels.size()); point++) {
            const int level = gammaPointLevels[static_cast<size_t>(point)];
            const QLineF distance(position, curvePoint(graph, level, curve[level]));
            if (distance.length() < nearestDistance) {
                nearestDistance = distance.length();
                nearest = point;
            }
        }
        return nearest;
    }

    void updatePointFromPosition(const QPointF &position) {
        if (m_selectedPoint < 0) {
            return;
        }
        const QRectF graph = graphRect();
        const float target = std::clamp(static_cast<float>((graph.bottom() - position.y()) / graph.height()), 0.0f, 1.0f);
        const int level = gammaPointLevels[static_cast<size_t>(m_selectedPoint)];
        const int maximum = gammaParameterMaximum(m_selectedPoint);
        int bestValue = gammaParameterValue(m_curves[m_activeCurve], m_selectedPoint);
        float bestDistance = std::numeric_limits<float>::max();
        for (int value = 0; value <= maximum; value++) {
            panel_gamma_t candidate = m_curves[m_activeCurve];
            setGammaParameterValue(candidate, m_selectedPoint, value);
            float curve[64];
            panel_get_gamma_curve(curve, &candidate);
            const float distance = std::abs(curve[level] - target);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestValue = value;
            }
        }
        setSelectedParameter(bestValue);
    }

    void setSelectedParameter(int value) {
        if (m_selectedPoint < 0) {
            return;
        }
        const int maximum = gammaParameterMaximum(m_selectedPoint);
        value = std::clamp(value, 0, maximum);
        if (gammaParameterValue(m_curves[m_activeCurve], m_selectedPoint) == value) {
            return;
        }
        setGammaParameterValue(m_curves[m_activeCurve], m_selectedPoint, value);
        update();
        notifySelectionChanged();
        if (changed) {
            changed();
        }
    }

    static void drawLegendItem(QPainter &painter, qreal x, qreal y, const QColor &color,
                               const QColor &textColor, const QString &text, bool dashed) {
        painter.setPen(QPen(color, 2, dashed ? Qt::DashLine : Qt::SolidLine));
        painter.drawLine(QPointF(x, y), QPointF(x + 22, y));
        painter.setPen(textColor);
        painter.drawText(QRectF(x + 27, y - 9, 68, 18), Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    void notifySelectionChanged() {
        if (selectionChanged) {
            selectionChanged();
        }
    }

    panel_gamma_t m_curves[2]{};
    int m_activeCurve = 0;
    int m_selectedPoint = -1;
    int m_hoverPoint = -1;
    bool m_dragging = false;
};

LcdDebugWidget::LcdDebugWidget(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("debugLcdWidget"));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *tabs = new QTabWidget(this);
    outer->addWidget(tabs);

    auto *pl111Page = new QWidget(tabs);
    auto *pl111Layout = new QVBoxLayout(pl111Page);

    m_pl111Editor = new QGroupBox(tr("Registers"), pl111Page);
    auto *pl111Form = new QFormLayout(m_pl111Editor);
    m_ppl = makeSpinBox(16, 1024, m_pl111Editor, 16);
    m_ppl->setToolTip(tr("PL111 PPL is encoded in multiples of 16 pixels."));
    m_hsw = makeSpinBox(1, 256, m_pl111Editor);
    m_hbp = makeSpinBox(1, 256, m_pl111Editor);
    m_cpl = makeSpinBox(1, 1024, m_pl111Editor);
    m_hfp = makeSpinBox(1, 256, m_pl111Editor);
    m_lpp = makeSpinBox(1, 1024, m_pl111Editor);
    m_vsw = makeSpinBox(1, 64, m_pl111Editor);
    m_vbp = makeSpinBox(0, 255, m_pl111Editor);
    m_vfp = makeSpinBox(0, 255, m_pl111Editor);
    m_pcd = makeSpinBox(1, 1025, m_pl111Editor);
    pl111Form->addRow(tr("Pixels per line (PPL)"), m_ppl);
    pl111Form->addRow(tr("Horizontal sync"), m_hsw);
    pl111Form->addRow(tr("Horizontal back porch"), m_hbp);
    pl111Form->addRow(tr("Active clocks (CPL)"), m_cpl);
    pl111Form->addRow(tr("Horizontal front porch"), m_hfp);
    pl111Form->addRow(tr("Lines per panel"), m_lpp);
    pl111Form->addRow(tr("Vertical sync"), m_vsw);
    pl111Form->addRow(tr("Vertical back porch"), m_vbp);
    pl111Form->addRow(tr("Vertical front porch"), m_vfp);
    pl111Form->addRow(tr("Pixel clock divider"), m_pcd);
    pl111Layout->addWidget(m_pl111Editor);

    auto *pl111Summary = new QGroupBox(tr("Effective timing"), pl111Page);
    auto *pl111SummaryLayout = new QVBoxLayout(pl111Summary);
    m_pl111Timing = makeSummaryLabel(pl111Summary);
    m_pl111Position = makeSummaryLabel(pl111Summary);
    pl111SummaryLayout->addWidget(m_pl111Timing);
    pl111SummaryLayout->addWidget(m_pl111Position);
    pl111Layout->addWidget(pl111Summary);
    pl111Layout->addStretch();
    tabs->addTab(makeScrollPage(pl111Page, tabs), tr("PL111"));

    auto *panelPage = new QWidget(tabs);
    auto *panelLayout = new QVBoxLayout(panelPage);
    m_panelEditor = new QGroupBox(tr("Commands"), panelPage);
    auto *panelEditorLayout = new QVBoxLayout(m_panelEditor);

    auto *addressScrollLayout = new QHBoxLayout;
    auto *windowGroup = new QGroupBox(tr("Address window"), m_panelEditor);
    auto *windowForm = new QFormLayout(windowGroup);
    m_colStart = makeSpinBox(0, PANEL_ADDR_MASK, windowGroup);
    m_colEnd = makeSpinBox(0, PANEL_ADDR_MASK, windowGroup);
    m_rowStart = makeSpinBox(0, PANEL_ADDR_MASK, windowGroup);
    m_rowEnd = makeSpinBox(0, PANEL_ADDR_MASK, windowGroup);
    windowForm->addRow(tr("Column start"), m_colStart);
    windowForm->addRow(tr("Column end"), m_colEnd);
    windowForm->addRow(tr("Row start"), m_rowStart);
    windowForm->addRow(tr("Row end"), m_rowEnd);
    addressScrollLayout->addWidget(windowGroup, 1);

    auto *scrollGroup = new QGroupBox(tr("Vertical scrolling"), m_panelEditor);
    auto *scrollForm = new QFormLayout(scrollGroup);
    m_scrollTop = makeSpinBox(0, PANEL_ADDR_MASK, scrollGroup);
    m_scrollArea = makeSpinBox(0, PANEL_ADDR_MASK, scrollGroup);
    m_scrollBottom = makeSpinBox(0, PANEL_ADDR_MASK, scrollGroup);
    m_scrollStart = makeSpinBox(0, PANEL_ADDR_MASK, scrollGroup);
    scrollForm->addRow(tr("Top fixed area"), m_scrollTop);
    scrollForm->addRow(tr("Scrollable area"), m_scrollArea);
    scrollForm->addRow(tr("Bottom fixed area"), m_scrollBottom);
    scrollForm->addRow(tr("Scroll start"), m_scrollStart);
    addressScrollLayout->addWidget(scrollGroup, 1);
    panelEditorLayout->addLayout(addressScrollLayout);

    auto *formatGroup = new QGroupBox(tr("Orientation and format"), m_panelEditor);
    auto *formatLayout = new QVBoxLayout(formatGroup);
    auto *orientationLayout = new QGridLayout;
    m_madctlMx = new QCheckBox(tr("Reverse columns (MX)"), formatGroup);
    m_madctlMy = new QCheckBox(tr("Reverse rows (MY)"), formatGroup);
    m_madctlMv = new QCheckBox(tr("Swap row/column (MV)"), formatGroup);
    m_madctlMl = new QCheckBox(tr("Reverse vertical refresh (ML)"), formatGroup);
    m_madctlMh = new QCheckBox(tr("Reverse horizontal refresh (MH)"), formatGroup);
    m_madctlRgb = new QCheckBox(tr("RGB order"), formatGroup);
    orientationLayout->addWidget(m_madctlMx, 0, 0);
    orientationLayout->addWidget(m_madctlMy, 0, 1);
    orientationLayout->addWidget(m_madctlMv, 1, 0);
    orientationLayout->addWidget(m_madctlMl, 1, 1);
    orientationLayout->addWidget(m_madctlMh, 2, 0);
    orientationLayout->addWidget(m_madctlRgb, 2, 1);
    formatLayout->addLayout(orientationLayout);
    auto *pixelFormatForm = new QFormLayout;
    m_mcuFormat = new QComboBox(formatGroup);
    m_rgbFormat = new QComboBox(formatGroup);
    appendPixelFormats(m_mcuFormat);
    appendPixelFormats(m_rgbFormat);
    pixelFormatForm->addRow(tr("MCU pixel format"), m_mcuFormat);
    pixelFormatForm->addRow(tr("RGB pixel format"), m_rgbFormat);
    formatLayout->addLayout(pixelFormatForm);
    panelEditorLayout->addWidget(formatGroup);

    auto *timingGroup = new QGroupBox(tr("Porch and frame timing"), m_panelEditor);
    auto *timingForm = new QFormLayout(timingGroup);
    m_panelVertBack = makeSpinBox(0, 127, timingGroup);
    m_panelVertFront = makeSpinBox(0, 127, timingGroup);
    m_rgbVertBack = makeSpinBox(0, 127, timingGroup);
    m_rgbHorizBack = makeSpinBox(0, 31, timingGroup);
    m_panelHorizFront = makeSpinBox(0, 31, timingGroup);
    m_panelClockDiv = makeSpinBox(0, 3, timingGroup);
    timingForm->addRow(tr("Normal vertical back porch"), m_panelVertBack);
    timingForm->addRow(tr("Normal vertical front porch"), m_panelVertFront);
    timingForm->addRow(tr("RGB vertical back porch"), m_rgbVertBack);
    timingForm->addRow(tr("RGB horizontal back porch"), m_rgbHorizBack);
    timingForm->addRow(tr("MCU horizontal front setting"), m_panelHorizFront);
    timingForm->addRow(tr("Panel clock divider"), m_panelClockDiv);
    panelEditorLayout->addWidget(timingGroup);

    auto *gammaGroup = new QGroupBox(tr("Gamma curves"), m_panelEditor);
    auto *gammaLayout = new QVBoxLayout(gammaGroup);
    auto *gammaToolbar = new QHBoxLayout;
    m_gammaPreset = new QComboBox(gammaGroup);
    for (int value = 0; value < 16; value++) {
        QString name = tr("Raw %1").arg(value);
        switch (value) {
            case 1: name = tr("Gamma 2.2 (%1)").arg(value); break;
            case 2: name = tr("Gamma 1.8 (%1)").arg(value); break;
            case 4: name = tr("Gamma 2.5 (%1)").arg(value); break;
            case 8: name = tr("Gamma 1.0 (%1)").arg(value); break;
            default: break;
        }
        m_gammaPreset->addItem(name, value);
    }
    auto *loadGammaPreset = new QPushButton(tr("Load preset"), gammaGroup);
    m_gammaCurve = new QComboBox(gammaGroup);
    m_gammaCurve->addItem(tr("Positive"), 0);
    m_gammaCurve->addItem(tr("Negative"), 1);
    gammaToolbar->addWidget(new QLabel(tr("Preset / GAMSET:"), gammaGroup));
    gammaToolbar->addWidget(m_gammaPreset, 1);
    gammaToolbar->addWidget(loadGammaPreset);
    gammaToolbar->addSpacing(12);
    gammaToolbar->addWidget(new QLabel(tr("Edit curve:"), gammaGroup));
    gammaToolbar->addWidget(m_gammaCurve);
    gammaLayout->addLayout(gammaToolbar);

    m_gammaEditor = new GammaCurveEditor(gammaGroup);
    gammaLayout->addWidget(m_gammaEditor);

    auto *gammaInterpolation = new QGridLayout;
    m_gammaPositiveJ0 = makeSpinBox(0, 3, gammaGroup);
    m_gammaPositiveJ1 = makeSpinBox(0, 3, gammaGroup);
    m_gammaNegativeJ0 = makeSpinBox(0, 3, gammaGroup);
    m_gammaNegativeJ1 = makeSpinBox(0, 3, gammaGroup);
    gammaInterpolation->addWidget(new QLabel(tr("Positive interpolation:"), gammaGroup), 0, 0);
    gammaInterpolation->addWidget(new QLabel(tr("J0"), gammaGroup), 0, 1);
    gammaInterpolation->addWidget(m_gammaPositiveJ0, 0, 2);
    gammaInterpolation->addWidget(new QLabel(tr("J1"), gammaGroup), 0, 3);
    gammaInterpolation->addWidget(m_gammaPositiveJ1, 0, 4);
    gammaInterpolation->setColumnStretch(5, 1);
    gammaInterpolation->addWidget(new QLabel(tr("Negative interpolation:"), gammaGroup), 0, 6);
    gammaInterpolation->addWidget(new QLabel(tr("J0"), gammaGroup), 0, 7);
    gammaInterpolation->addWidget(m_gammaNegativeJ0, 0, 8);
    gammaInterpolation->addWidget(new QLabel(tr("J1"), gammaGroup), 0, 9);
    gammaInterpolation->addWidget(m_gammaNegativeJ1, 0, 10);
    gammaLayout->addLayout(gammaInterpolation);

    m_gammaSelection = new QLabel(gammaGroup);
    m_gammaSelection->setWordWrap(true);
    m_gammaSelection->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gammaLayout->addWidget(m_gammaSelection);

    const auto previewPreset = [this] {
        panel_gamma_t positive, negative;
        panel_get_gamma_preset(static_cast<uint8_t>(m_gammaPreset->currentData().toInt()), &positive, &negative);
        m_gammaEditor->setCurves(positive, negative);
        updateGammaInterpolationEditors();
    };
    connect(m_gammaPreset, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [previewPreset](int) { previewPreset(); });
    connect(loadGammaPreset, &QPushButton::clicked, this, previewPreset);
    connect(m_gammaCurve, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) { m_gammaEditor->setActiveCurve(m_gammaCurve->itemData(index).toInt()); });
    const auto updatePositiveInterpolation = [this] {
        m_gammaEditor->setInterpolation(0, m_gammaPositiveJ0->value(), m_gammaPositiveJ1->value());
    };
    const auto updateNegativeInterpolation = [this] {
        m_gammaEditor->setInterpolation(1, m_gammaNegativeJ0->value(), m_gammaNegativeJ1->value());
    };
    connect(m_gammaPositiveJ0, qOverload<int>(&QSpinBox::valueChanged), this,
            [updatePositiveInterpolation](int) { updatePositiveInterpolation(); });
    connect(m_gammaPositiveJ1, qOverload<int>(&QSpinBox::valueChanged), this,
            [updatePositiveInterpolation](int) { updatePositiveInterpolation(); });
    connect(m_gammaNegativeJ0, qOverload<int>(&QSpinBox::valueChanged), this,
            [updateNegativeInterpolation](int) { updateNegativeInterpolation(); });
    connect(m_gammaNegativeJ1, qOverload<int>(&QSpinBox::valueChanged), this,
            [updateNegativeInterpolation](int) { updateNegativeInterpolation(); });
    m_gammaEditor->changed = [this] {
        updateGammaInterpolationEditors();
        m_gammaSelection->setText(m_gammaEditor->selectionText());
    };
    m_gammaEditor->selectionChanged = [this] {
        m_gammaSelection->setText(m_gammaEditor->selectionText());
    };
    previewPreset();
    panelEditorLayout->addWidget(gammaGroup);
    panelLayout->addWidget(m_panelEditor);

    auto *panelSummary = new QGroupBox(tr("Effective timing"), panelPage);
    auto *panelSummaryLayout = new QVBoxLayout(panelSummary);
    m_panelTiming = makeSummaryLabel(panelSummary);
    m_panelState = makeSummaryLabel(panelSummary);
    panelSummaryLayout->addWidget(m_panelTiming);
    panelSummaryLayout->addWidget(m_panelState);
    panelLayout->addWidget(panelSummary);
    panelLayout->addStretch();
    tabs->addTab(makeScrollPage(panelPage, tabs), tr("ST7789"));

    setEditingEnabled(false);
}

void LcdDebugWidget::setEditingEnabled(bool enabled) {
    m_pl111Editor->setEnabled(enabled);
    m_panelEditor->setEnabled(enabled);
}

void LcdDebugWidget::updateGammaInterpolationEditors() {
    const panel_gamma_t &positive = m_gammaEditor->positiveCurve();
    const panel_gamma_t &negative = m_gammaEditor->negativeCurve();
    const QSignalBlocker positiveJ0Block(m_gammaPositiveJ0);
    const QSignalBlocker positiveJ1Block(m_gammaPositiveJ1);
    const QSignalBlocker negativeJ0Block(m_gammaNegativeJ0);
    const QSignalBlocker negativeJ1Block(m_gammaNegativeJ1);
    m_gammaPositiveJ0->setValue(positive.J0);
    m_gammaPositiveJ1->setValue(positive.J1);
    m_gammaNegativeJ0->setValue(negative.J0);
    m_gammaNegativeJ1->setValue(negative.J1);
}

void LcdDebugWidget::populate() {
    const uint32_t timing0 = lcd.timing[0];
    const uint32_t timing1 = lcd.timing[1];
    const uint32_t timing2 = lcd.timing[2];
    const int ppl = static_cast<int>(((timing0 >> 2 & 0x3F) + 1) << 4);
    const int hsw = static_cast<int>((timing0 >> 8 & 0xFF) + 1);
    const int hfp = static_cast<int>((timing0 >> 16 & 0xFF) + 1);
    const int hbp = static_cast<int>((timing0 >> 24 & 0xFF) + 1);
    const int lpp = static_cast<int>((timing1 & 0x3FF) + 1);
    const int vsw = static_cast<int>((timing1 >> 10 & 0x3F) + 1);
    const int vfp = static_cast<int>(timing1 >> 16 & 0xFF);
    const int vbp = static_cast<int>(timing1 >> 24 & 0xFF);
    const int cpl = static_cast<int>((timing2 >> 16 & 0x3FF) + 1);
    const int pcd = timing2 >> 26 & 1 ? 1 :
        static_cast<int>(((timing2 & 0x1F) | (timing2 >> 27 & 0x1F) << 5) + 2);

    m_ppl->setValue(ppl);
    m_hsw->setValue(hsw);
    m_hbp->setValue(hbp);
    m_cpl->setValue(cpl);
    m_hfp->setValue(hfp);
    m_lpp->setValue(lpp);
    m_vsw->setValue(vsw);
    m_vbp->setValue(vbp);
    m_vfp->setValue(vfp);
    m_pcd->setValue(pcd);

    const uint32_t horizTotal = static_cast<uint32_t>(hsw + hbp + cpl + hfp);
    const uint32_t vertTotal = static_cast<uint32_t>(vsw + vbp + lpp + vfp);
    const double pixelClock = 24000000.0 / pcd;
    const double lineRate = horizTotal ? pixelClock / horizTotal : 0.0;
    const double frameRate = vertTotal ? lineRate / vertTotal : 0.0;
    m_pl111Timing->setText(tr("Horizontal: %1 sync + %2 back + %3 active + %4 front = %5 clocks\n"
                               "Vertical: %6 sync + %7 back + %8 active + %9 front = %10 lines\n"
                               "Pixel clock: %11 MHz; line rate: %12 kHz; frame rate: %13 Hz")
        .arg(hsw).arg(hbp).arg(cpl).arg(hfp).arg(horizTotal)
        .arg(vsw).arg(vbp).arg(lpp).arg(vfp).arg(vertTotal)
        .arg(pixelClock / 1e6, 0, 'f', 3).arg(lineRate / 1e3, 0, 'f', 3).arg(frameRate, 0, 'f', 3));

    const QString dmaAddress = QStringLiteral("%1").arg(lcd.upcurr, 6, 16, QLatin1Char('0')).toUpper();
    m_pl111Position->setText(tr("Current: %1, row %2, column %3; DMA address: %4")
        .arg(lcdPhaseText(lcd.compare))
        .arg(lcd.curRow).arg(lcd.curCol)
        .arg(dmaAddress));

    m_colStart->setValue(panel.params.CASET.XS);
    m_colEnd->setValue(panel.params.CASET.XE);
    m_rowStart->setValue(panel.params.RASET.YS);
    m_rowEnd->setValue(panel.params.RASET.YE);
    m_scrollTop->setValue(panel.params.VSCRDEF.TFA);
    m_scrollArea->setValue(panel.params.VSCRDEF.VSA);
    m_scrollBottom->setValue(panel.params.VSCRDEF.BFA);
    m_scrollStart->setValue(panel.params.VSCRSADD.VSP);
    m_madctlMx->setChecked(panel.params.MADCTL.MX);
    m_madctlMy->setChecked(panel.params.MADCTL.MY);
    m_madctlMv->setChecked(panel.params.MADCTL.MV);
    m_madctlMl->setChecked(panel.params.MADCTL.ML);
    m_madctlMh->setChecked(panel.params.MADCTL.MH);
    m_madctlRgb->setChecked(panel.params.MADCTL.RGB);
    m_mcuFormat->setCurrentIndex(panel.params.COLMOD.MCU);
    m_rgbFormat->setCurrentIndex(panel.params.COLMOD.RGB);
    m_panelVertBack->setValue(panel.params.PORCTRL.BPA);
    m_panelVertFront->setValue(panel.params.PORCTRL.FPA);
    m_rgbVertBack->setValue(panel.params.RGBCTRL.VBP);
    m_rgbHorizBack->setValue(panel.params.RGBCTRL.HBP);
    m_panelHorizFront->setValue(panel.params.FRCTRL2.RTNA);
    m_panelClockDiv->setValue(panel.params.FRCTRL1.DIV);
    {
        const QSignalBlocker gammaPresetBlock(m_gammaPreset);
        m_gammaPreset->setCurrentIndex(m_gammaPreset->findData(panel.params.GAMSET.GC));
    }
    m_gammaEditor->setCurves(panel.params.PVGAMCTRL, panel.params.NVGAMCTRL);
    updateGammaInterpolationEditors();
    m_gammaSelection->setText(m_gammaEditor->selectionText());

    panel_timing_t panelTiming;
    panel_get_timing(&panelTiming);
    const uint32_t panelHorizTotal = panelTiming.horizBackPorch + panelTiming.horizActive + panelTiming.horizFrontPorch;
    const uint32_t panelVertTotal = panelTiming.vertBackPorch + panelTiming.vertActive + panelTiming.vertFrontPorch;
    const uint32_t panelClock = panel.clockRate >> panel.params.FRCTRL1.DIV;
    const double panelLineRate = panelHorizTotal ? static_cast<double>(panelClock) / panelHorizTotal : 0.0;
    const double panelFrameRate = panelVertTotal ? panelLineRate / panelVertTotal : 0.0;
    m_panelTiming->setText(tr("Horizontal: %1 back + %2 active + %3 front = %4 ticks\n"
                              "Vertical: %5 back + %6 active + %7 front = %8 lines\n"
                              "Panel clock: %9 MHz; line rate: %10 kHz; frame rate: %11 Hz")
        .arg(panelTiming.horizBackPorch).arg(panelTiming.horizActive).arg(panelTiming.horizFrontPorch).arg(panelHorizTotal)
        .arg(panelTiming.vertBackPorch).arg(panelTiming.vertActive).arg(panelTiming.vertFrontPorch).arg(panelVertTotal)
        .arg(panelClock / 1e6, 0, 'f', 3).arg(panelLineRate / 1e3, 0, 'f', 3).arg(panelFrameRate, 0, 'f', 3));

    QStringList flags;
    if (panel.mode & PANEL_MODE_SLEEP) flags << tr("sleep");
    if (panel.mode & PANEL_MODE_OFF) flags << tr("display off");
    if (panel.mode & PANEL_MODE_BLANK) flags << tr("blank");
    if (panel.mode & PANEL_MODE_PARTIAL) flags << tr("partial");
    if (panel.mode & PANEL_MODE_IDLE) flags << tr("idle");
    if (panel.mode & PANEL_MODE_SCROLL) flags << tr("scroll");
    if (panel.invert) flags << tr("inverted");
    m_panelState->setText(tr("Mode: %1; row %2, column %3; flags: %4")
        .arg(panelModeText(panel.displayMode)).arg(panel.row).arg(panel.col)
        .arg(flags.isEmpty() ? tr("none") : flags.join(QStringLiteral(", "))));
}

void LcdDebugWidget::sync() const {
    uint32_t timing0 = lcd.timing[0];
    const int ppl = std::clamp((m_ppl->value() + 8) / 16 * 16, 16, 1024);
    timing0 &= ~UINT32_C(0xFFFFFFFC);
    timing0 |= static_cast<uint32_t>((ppl / 16 - 1) & 0x3F) << 2;
    timing0 |= static_cast<uint32_t>(m_hsw->value() - 1) << 8;
    timing0 |= static_cast<uint32_t>(m_hfp->value() - 1) << 16;
    timing0 |= static_cast<uint32_t>(m_hbp->value() - 1) << 24;
    lcd.timing[0] = timing0;

    uint32_t timing1 = 0;
    timing1 |= static_cast<uint32_t>(m_lpp->value() - 1);
    timing1 |= static_cast<uint32_t>(m_vsw->value() - 1) << 10;
    timing1 |= static_cast<uint32_t>(m_vfp->value()) << 16;
    timing1 |= static_cast<uint32_t>(m_vbp->value()) << 24;
    lcd.timing[1] = timing1;

    uint32_t timing2 = lcd.timing[2];
    timing2 &= ~UINT32_C(0xFFFF001F);
    const uint32_t pcd = static_cast<uint32_t>(m_pcd->value());
    if (pcd == 1) {
        timing2 |= UINT32_C(1) << 26;
    } else {
        const uint32_t encoded = pcd - 2;
        timing2 |= encoded & 0x1F;
        timing2 |= (encoded >> 5 & 0x1F) << 27;
    }
    timing2 |= static_cast<uint32_t>(m_cpl->value() - 1) << 16;
    lcd.timing[2] = timing2;

    if (panel.params.CASET.XS != m_colStart->value() || panel.params.CASET.XE != m_colEnd->value()) {
        const auto start = wordBytes(static_cast<uint16_t>(m_colStart->value()));
        const auto end = wordBytes(static_cast<uint16_t>(m_colEnd->value()));
        writeCommand(0x2A, std::array<uint8_t, 4>{ start[0], start[1], end[0], end[1] });
    }
    if (panel.params.RASET.YS != m_rowStart->value() || panel.params.RASET.YE != m_rowEnd->value()) {
        const auto start = wordBytes(static_cast<uint16_t>(m_rowStart->value()));
        const auto end = wordBytes(static_cast<uint16_t>(m_rowEnd->value()));
        writeCommand(0x2B, std::array<uint8_t, 4>{ start[0], start[1], end[0], end[1] });
    }
    if (panel.params.VSCRDEF.TFA != m_scrollTop->value() ||
        panel.params.VSCRDEF.VSA != m_scrollArea->value() ||
        panel.params.VSCRDEF.BFA != m_scrollBottom->value()) {
        const auto top = wordBytes(static_cast<uint16_t>(m_scrollTop->value()));
        const auto area = wordBytes(static_cast<uint16_t>(m_scrollArea->value()));
        const auto bottom = wordBytes(static_cast<uint16_t>(m_scrollBottom->value()));
        writeCommand(0x33, std::array<uint8_t, 6>{ top[0], top[1], area[0], area[1], bottom[0], bottom[1] });
    }
    if (panel.params.VSCRSADD.VSP != m_scrollStart->value()) {
        writeCommand(0x37, wordBytes(static_cast<uint16_t>(m_scrollStart->value())));
    }

    auto madctl = panel.params.MADCTL;
    madctl.MX = m_madctlMx->isChecked();
    madctl.MY = m_madctlMy->isChecked();
    madctl.MV = m_madctlMv->isChecked();
    madctl.ML = m_madctlMl->isChecked();
    madctl.MH = m_madctlMh->isChecked();
    madctl.RGB = m_madctlRgb->isChecked();
    if (memcmp(&madctl, &panel.params.MADCTL, sizeof(madctl))) {
        writeCommand(0x36, madctl);
    }

    auto colmod = panel.params.COLMOD;
    colmod.MCU = static_cast<uint8_t>(m_mcuFormat->currentData().toInt());
    colmod.RGB = static_cast<uint8_t>(m_rgbFormat->currentData().toInt());
    if (memcmp(&colmod, &panel.params.COLMOD, sizeof(colmod))) {
        writeCommand(0x3A, colmod);
    }

    auto porctrl = panel.params.PORCTRL;
    porctrl.BPA = static_cast<uint8_t>(m_panelVertBack->value());
    porctrl.FPA = static_cast<uint8_t>(m_panelVertFront->value());
    if (memcmp(&porctrl, &panel.params.PORCTRL, sizeof(porctrl))) {
        writeCommand(0xB2, porctrl);
    }

    auto rgbctrl = panel.params.RGBCTRL;
    rgbctrl.VBP = static_cast<uint8_t>(m_rgbVertBack->value());
    rgbctrl.HBP = static_cast<uint8_t>(m_rgbHorizBack->value());
    if (memcmp(&rgbctrl, &panel.params.RGBCTRL, sizeof(rgbctrl))) {
        writeCommand(0xB1, rgbctrl);
    }

    auto frctrl1 = panel.params.FRCTRL1;
    frctrl1.DIV = static_cast<uint8_t>(m_panelClockDiv->value());
    if (memcmp(&frctrl1, &panel.params.FRCTRL1, sizeof(frctrl1))) {
        writeCommand(0xB3, frctrl1);
    }

    auto frctrl2 = panel.params.FRCTRL2;
    frctrl2.RTNA = static_cast<uint8_t>(m_panelHorizFront->value());
    if (memcmp(&frctrl2, &panel.params.FRCTRL2, sizeof(frctrl2))) {
        writeCommand(0xC6, frctrl2);
    }

    auto gamset = panel.params.GAMSET;
    gamset.GC = static_cast<uint8_t>(m_gammaPreset->currentData().toInt());
    if (memcmp(&gamset, &panel.params.GAMSET, sizeof(gamset))) {
        writeCommand(0x26, gamset);
    }

    const panel_gamma_t positiveGamma = m_gammaEditor->positiveCurve();
    if (memcmp(&positiveGamma, &panel.params.PVGAMCTRL, sizeof(positiveGamma))) {
        writeCommand(0xE0, positiveGamma);
    }
    const panel_gamma_t negativeGamma = m_gammaEditor->negativeCurve();
    if (memcmp(&negativeGamma, &panel.params.NVGAMCTRL, sizeof(negativeGamma))) {
        writeCommand(0xE1, negativeGamma);
    }
}
