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

    QStringList setup;
    Ui::MemoryVisualizer *ui;

    int scale = 100;
    int rate = 30;

    // LCD configuration
    uint32_t height;
    uint32_t width;
    uint32_t upbase;
    uint32_t control;
    uint32_t *data = NULL;
    uint32_t *data_end = NULL;
};

#endif
