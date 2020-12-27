#include "corewindow.h"
#include "calculatorwidget.h"

#include <kddockwidgets/LayoutSaver.h>

#include <QMenu>
#include <QMenuBar>
#include <QEvent>
#include <QDebug>
#include <QString>
#include <QTextEdit>
#include <QApplication>

CoreWindow::CoreWindow(const QString &uniqueName,
                       KDDockWidgets::MainWindowOptions options,
                       QWidget *parent)
    : KDDockWidgets::MainWindow(uniqueName, options, parent)
{
    setContentsMargins(0, 0, 0, 0);

    auto menubar = menuBar();
    auto fileMenu = new QMenu(tr("File"));
    menubar->addMenu(fileMenu);

    auto saveLayoutAction = fileMenu->addAction(tr("Save Layout"));
    connect(saveLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::LayoutSaver saver;
        const bool result = saver.saveToFile(QStringLiteral("mylayout.json"));
        qDebug() << "Saving layout to disk. Result=" << result;
    });

    auto restoreLayoutAction = fileMenu->addAction(tr("Restore Layout"));
    connect(restoreLayoutAction, &QAction::triggered, this, [] {
        KDDockWidgets::RestoreOptions options = KDDockWidgets::RestoreOption_None;
        KDDockWidgets::LayoutSaver saver(options);
        saver.restoreFromFile(QStringLiteral("mylayout.json"));
    });

    auto quitAction = fileMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    createDockWidgets();
}

CoreWindow::~CoreWindow()
{
    qDeleteAll(m_dockwidgets);
}

void CoreWindow::createDockWidgets()
{
    Q_ASSERT(m_dockwidgets.isEmpty());

    auto *calcDock = new KDDockWidgets::DockWidget(tr("Calculator"));
    auto *calc = new CalculatorWidget();

    calc->setType(ti_device_t::TI83PCE);
    calcDock->setWidget(calc);

    m_dockwidgets << calcDock;

    addDockWidget(calcDock, KDDockWidgets::Location_OnTop);
}
