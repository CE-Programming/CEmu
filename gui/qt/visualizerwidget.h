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
    void setDefaultView();
    void showHelp();
    void showPresets();

private:
    void stringToView();
    void viewToString();

    QStringList m_setup;

    int m_scale = 100;
    int m_rate = 30;

    uint32_t m_height;          // lcd configuration
    uint32_t m_width;
    uint32_t m_base;
    uint32_t m_control;

    VisualizerDisplayWidget *m_view;
    QLineEdit *m_config;
    QGroupBox *m_group;

    QToolButton *m_btnLcd;
    QToolButton *m_btnDebug;
    QToolButton *m_btnInfo;
};


#endif
