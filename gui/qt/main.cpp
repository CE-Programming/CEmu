#include "corewindow.h"
#include "screenwidget.h"
#include "dockwidget.h"

#include <kddockwidgets/Config.h>

#include <QStyleFactory>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    KDDockWidgets::MainWindowOptions options = KDDockWidgets::MainWindowOption_None;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);

    app.setOrganizationName(QStringLiteral("cemu-dev"));
    app.setApplicationName(QStringLiteral("CEmu"));

    KDDockWidgets::Config::self().setFrameworkWidgetFactory(new DockWidgetFactory());
    KDDockWidgets::Config::self().setSeparatorThickness(0);

    auto flags = KDDockWidgets::Config::self().flags();
    flags |= KDDockWidgets::Config::Flag_AlwaysTitleBarWhenFloating;
    flags |= KDDockWidgets::Config::Flag_TabsHaveCloseButton;
    KDDockWidgets::Config::self().setFlags(flags);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();

    CoreWindow window(QStringLiteral("CEmu"), options);
    window.setWindowTitle(QStringLiteral("CEmu"));
    window.resize(screenGeometry.height() * .325, screenGeometry.height() * .8);
    window.show();

    return app.exec();
}
