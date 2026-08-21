#ifndef LCDDEBUGWIDGET_H
#define LCDDEBUGWIDGET_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QSpinBox;
class GammaCurveEditor;

class LcdDebugWidget : public QWidget {
    Q_OBJECT

public:
    explicit LcdDebugWidget(QWidget *parent = nullptr);

    void populate();
    void setEditingEnabled(bool enabled);
    void sync() const;

private:
    QGroupBox *m_pl111Editor;
    QSpinBox *m_ppl;
    QSpinBox *m_hsw;
    QSpinBox *m_hbp;
    QSpinBox *m_cpl;
    QSpinBox *m_hfp;
    QSpinBox *m_lpp;
    QSpinBox *m_vsw;
    QSpinBox *m_vbp;
    QSpinBox *m_vfp;
    QSpinBox *m_pcd;
    QLabel *m_pl111Timing;
    QLabel *m_pl111Position;

    QGroupBox *m_panelEditor;
    QSpinBox *m_colStart;
    QSpinBox *m_colEnd;
    QSpinBox *m_rowStart;
    QSpinBox *m_rowEnd;
    QSpinBox *m_scrollTop;
    QSpinBox *m_scrollArea;
    QSpinBox *m_scrollBottom;
    QSpinBox *m_scrollStart;
    QCheckBox *m_madctlMx;
    QCheckBox *m_madctlMy;
    QCheckBox *m_madctlMv;
    QCheckBox *m_madctlMl;
    QCheckBox *m_madctlMh;
    QCheckBox *m_madctlRgb;
    QComboBox *m_mcuFormat;
    QComboBox *m_rgbFormat;
    QSpinBox *m_panelVertBack;
    QSpinBox *m_panelVertFront;
    QSpinBox *m_rgbVertBack;
    QSpinBox *m_rgbHorizBack;
    QSpinBox *m_panelHorizFront;
    QSpinBox *m_panelClockDiv;
    QComboBox *m_gammaPreset;
    QComboBox *m_gammaCurve;
    QSpinBox *m_gammaPositiveJ0;
    QSpinBox *m_gammaPositiveJ1;
    QSpinBox *m_gammaNegativeJ0;
    QSpinBox *m_gammaNegativeJ1;
    GammaCurveEditor *m_gammaEditor;
    QLabel *m_gammaSelection;
    QLabel *m_panelTiming;
    QLabel *m_panelState;

    void updateGammaInterpolationEditors();
};

#endif
