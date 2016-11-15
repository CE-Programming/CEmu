#ifndef LCDPOPOUT_H
#define LCDPOPOUT_H

#include <QtWidgets/QDialog>

#include "../../core/lcd.h"
#include "keypad/qtkeypadbridge.h"

namespace Ui { class LCDPopout; }

class LCDPopout : public QDialog {
    Q_OBJECT

public:
    explicit LCDPopout(QtKeypadBridge *bridge, QWidget *p = Q_NULLPTR);
    ~LCDPopout();

    // Misc.
    virtual void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;

    void changeAddress();
    void changeBPP();
    void changeChecked();

private:
    Ui::LCDPopout *ui;
    lcd_state_t lcdState;
};

#endif // LCDPOPOUT_H
