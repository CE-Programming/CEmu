#include "calculatorwidget.h"

#include <QVBoxLayout>

CalculatorWidget::CalculatorWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_keypad = new KeypadWidget(this);
    m_screen = new ScreenWidget(this);

    setFocusPolicy(Qt::StrongFocus);

    m_keypad->setMinimumSize(50, 50);
    m_screen->setMinimumSize(50, 28);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addWidget(m_screen);
    layout->addWidget(m_keypad);
    layout->setStretch(0, 35);
    layout->setStretch(1, 65);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

CalculatorWidget::~CalculatorWidget()
{
}

void CalculatorWidget::setType(ti_device_t type)
{
    switch (type)
    {
        default:
            m_keypad->setType(false, KeypadWidget::COLOR_RED);
            m_screen->setSkin(QStringLiteral(":/assets/skin/ti84pce.png"));
            break;

        case ti_device_t::TI83PCE:
            m_keypad->setType(true, KeypadWidget::COLOR_DENIM);
            m_screen->setSkin(QStringLiteral(":/assets/skin/ti83pce.png"));
            break;
    }
}


