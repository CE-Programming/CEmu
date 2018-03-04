#ifndef MEMORYVISUALIZER_H
#define MEMORYVISUALIZER_H

#include "keypad/qtkeypadbridge.h"
#include "../../core/lcd.h"

#include <QtWidgets/QDialog>

namespace Ui { class MemoryVisualizer; }

class MemoryVisualizer : public QDialog {
    Q_OBJECT

public:
    explicit MemoryVisualizer(QWidget *p = Q_NULLPTR);
    ~MemoryVisualizer();

protected:
    virtual void keyPressEvent(QKeyEvent*) Q_DECL_OVERRIDE;

private slots:
    void setDefaultView();
    void showHelp();

private:
    void stringToView();
    void viewToString();

    QStringList m_setup;
    Ui::MemoryVisualizer *ui;

    int m_scale = 100;
    int m_rate = 30;

    uint32_t m_height;          // lcd configuration
    uint32_t m_width;
    uint32_t m_base;
    uint32_t m_control;
};

#endif
