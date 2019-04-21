#ifndef MEMORYVISUALIZER_H
#define MEMORYVISUALIZER_H

#include "keypad/qtkeypadbridge.h"
#include "debugger/visualizerdisplaywidget.h"
#include "../../core/lcd.h"

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QToolButton>

class VisualizerWidget : public QWidget {
    Q_OBJECT

public:
    explicit VisualizerWidget(QWidget *parent = Q_NULLPTR, const QString &config = QString());
    ~VisualizerWidget();
    QString getConfig();
    void translate();
    void forceUpdate();

signals:
    void configChanged();

private slots:
    void resetView();
    void showConfig();
    void showPresets();

private:
    void stringToView();
    void viewToString();

    QStringList m_setup;

    int m_scale = 100;
    int m_fps = 30;

    int m_height;          // lcd configuration
    int m_width;
    uint32_t m_base;
    uint32_t m_control;
    bool m_grid;

    VisualizerDisplayWidget *m_view;
    QLineEdit *m_config;
    QGroupBox *m_group;

    QToolButton *m_btnLcd;
    QToolButton *m_btnRefresh;
    QToolButton *m_btnConfig;
};

#endif
