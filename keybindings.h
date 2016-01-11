#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <QtWidgets/QDialog>

namespace Ui {
    class KeyBindings;
}

class KeyBindings : public QDialog {
    Q_OBJECT

public:
    explicit KeyBindings(QWidget *p = 0);
    ~KeyBindings();

private:
    Ui::KeyBindings *ui;
};

#endif // KEYBINDINGS_H
