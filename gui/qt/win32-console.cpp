#ifdef _WIN32
#include <QtWidgets/QAction>
#include <QtWidgets/QMessageBox>
#include <windows.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::toggleConsole() {
    if (actionToggleConsole->isChecked()) {
        // If the console is created from opening up the EXE in
        // Explorer, the console will be completely destroyed once the
        // console is hidden/freed. Therefore, we need to check for
        // that, and create a new console if necessary. On the other
        // hand, if we spawned this process from an existing console,
        // AttachConsole will still work!
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            if (!AllocConsole()) {
                QMessageBox::critical(this, "Error", "Unable to open console.");
            }
        }
    } else {
        if (!FreeConsole()) {
            QMessageBox::critical(this, "Error", "Unable to close console. If you are running directly from a console, you may not be able to close it.");
        }
    }
    
    settings->setValue(SETTING_ENABLE_WIN_CONSOLE, actionToggleConsole->isChecked());
}

void MainWindow::installToggleConsole() {
    // Build menu option and add it!
    actionToggleConsole = new QAction(TXT_TOGGLE_CONSOLE, this);
    actionToggleConsole->setObjectName(QStringLiteral("actionToggleConsole"));
    actionToggleConsole->setCheckable(true);
    actionToggleConsole->setChecked(true);
    actionToggleConsole->setEnabled(true);
    ui->menuCalculator->addAction(actionToggleConsole);
    
    // Connect menu action to function
    connect(actionToggleConsole, &QAction::triggered, this, &MainWindow::toggleConsole);
    
    // Check if we opted to not show a window
    if (!settings->value(SETTING_ENABLE_WIN_CONSOLE, true).toBool()) {
        actionToggleConsole->setChecked(false);
        toggleConsole();
    }
}
#endif
