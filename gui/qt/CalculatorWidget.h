#ifndef CALCULATORWIDGET_H
#define CALCULATORWIDGET_H

#include <QWidget>

class CalculatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CalculatorWidget(QWidget *parent = nullptr);
    ~CalculatorWidget();
};

#endif
