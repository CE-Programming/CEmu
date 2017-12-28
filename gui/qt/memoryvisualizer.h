#ifndef MEMORYVISUALIZER_H
#define MEMORYVISUALIZER_H

#include <QtWidgets/QDialog>

#include "../../core/lcd.h"
#include "keypad/qtkeypadbridge.h"

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

    lcd_state_t mView;
    QStringList setup;

    int scale = 100;
    int rate = 30;

    Ui::MemoryVisualizer *ui;
};

#endif
