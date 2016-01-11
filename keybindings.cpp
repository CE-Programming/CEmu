#include "keybindings.h"
#include "ui_keybindings.h"

KeyBindings::KeyBindings(QWidget *p) : QDialog(p), ui(new Ui::KeyBindings) {
    ui->setupUi(this);
}

KeyBindings::~KeyBindings() {
    delete ui;
}
