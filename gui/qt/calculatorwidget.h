#ifndef CALCULATORWIDGET_H
#define CALCULATORWIDGET_H

#include <QWidget>
#include <QResizeEvent>

#include "keypad/keypadwidget.h"
#include "screenwidget.h"

#include "../../core/asic.h"

class CalculatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CalculatorWidget(QWidget *parent = nullptr);
    ~CalculatorWidget();

    void setType(ti_device_t type);

private:
    KeypadWidget *m_keypad;
    ScreenWidget *m_screen;
};

#endif
