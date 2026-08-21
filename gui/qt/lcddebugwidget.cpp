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
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>

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
    (void)panel_debug_write_command(command, params.data(), params.size());
}

template <typename T>
void writeCommand(uint8_t command, const T &params) {
    (void)panel_debug_write_command(command, reinterpret_cast<const uint8_t *>(&params), sizeof(params));
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

} // namespace

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
    panelEditorLayout->addWidget(windowGroup);

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
    panelEditorLayout->addWidget(scrollGroup);

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

    auto *gammaGroup = new QGroupBox(tr("Gamma"), m_panelEditor);
    auto *gammaForm = new QFormLayout(gammaGroup);
    m_gammaPreset = new QComboBox(gammaGroup);
    for (int value = 0; value < 16; value++) {
        QString name = tr("Raw %1").arg(value);
        if (value == 1 || value == 2 || value == 4 || value == 8) {
            name = tr("Preset %1").arg(value);
        }
        m_gammaPreset->addItem(name, value);
    }
    gammaForm->addRow(tr("Gamma curve (GAMSET)"), m_gammaPreset);
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
    m_gammaPreset->setCurrentIndex(panel.params.GAMSET.GC);

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
}
