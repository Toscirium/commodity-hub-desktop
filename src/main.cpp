#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QIcon>

#include "core/Config.h"
#include "core/Session.h"
#include "core/SupabaseClient.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    // Required by QtWebEngineWidgets (used to render the TradingView chart in
    // ChartPanel) before any QGuiApplication/QApplication is constructed.
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName(Config::ApplicationName);
    app.setOrganizationName(Config::OrganizationName);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    // Bundled so the desktop app renders with the same typefaces as the web
    // app (DM Sans body / Space Grotesk display / JetBrains Mono data) even on
    // machines (e.g. Windows) that don't have them installed.
    QFontDatabase::addApplicationFont(":/fonts/DMSans.ttf");
    QFontDatabase::addApplicationFont(":/fonts/SpaceGrotesk.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono.ttf");
    app.setFont(QFont(QStringLiteral("DM Sans"), 10));

    QFile styleFile(":/theme.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    Session session;
    session.load();

    SupabaseClient client(session);

    if (!session.isValid()) {
        LoginDialog loginDialog(client);
        if (loginDialog.exec() != QDialog::Accepted)
            return 0;
    }

    // MainWindow hides to a tray icon on close (when one is available) rather
    // than exiting, so the app can keep polling for price alerts in the
    // background; only its own Quit action should end the event loop.
    app.setQuitOnLastWindowClosed(false);

    MainWindow window(session, client);
    window.show();

    return app.exec();
}
