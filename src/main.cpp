#include "mpris.h"
#include "netfactory.h"
#include "player.h"
#include "somafm.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("lighthugger"));
    QApplication::setApplicationName(QStringLiteral("lighthugger"));
    QApplication::setApplicationDisplayName(QStringLiteral("Lighthugger"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.mrdfuse.lighthugger"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.mrdfuse.lighthugger"),
                                                 QIcon(QStringLiteral(":/data/io.github.mrdfuse.lighthugger.svg"))));

    // Carry favorites and settings over from when the app was called somafm-player.
    {
        QSettings settings;
        if (settings.allKeys().isEmpty()) {
            QSettings old(QStringLiteral("somafm-player"), QStringLiteral("somafm-player"));
            for (const QString &key : old.allKeys())
                settings.setValue(key, old.value(key));
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Lighthugger: a player for SomaFM radio stations"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("station"), QStringLiteral("Station id to start playing, e.g. groovesalad"));
    parser.process(app);
    const QString station = parser.positionalArguments().value(0);

    // Use the native Plasma (Breeze) look for Qt Quick Controls.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    SomaFM soma;
    Player player(&soma);

    if (!registerMpris(&player)) {
        // Already running: bring the existing window to the front instead.
        QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(
            QStringLiteral("org.mpris.MediaPlayer2.lighthugger"), QStringLiteral("/org/mpris/MediaPlayer2"),
            QStringLiteral("org.mpris.MediaPlayer2"), QStringLiteral("Raise")));
        return 0;
    }

    NetFactory netFactory; // must outlive the engine
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&netFactory);
    qmlRegisterSingletonInstance("org.somafm", 1, 0, "Soma", &soma);
    qmlRegisterSingletonInstance("org.somafm", 1, 0, "Player", &player);
    qmlRegisterUncreatableType<ChannelFilter>("org.somafm", 1, 0, "ChannelFilter", QStringLiteral("Provided by Soma"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
                     Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (!station.isEmpty()) {
        if (soma.find(station)) {
            player.play(station);
        } else {
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = QObject::connect(&soma, &SomaFM::channelsUpdated, &player, [&player, station, conn] {
                QObject::disconnect(*conn);
                player.play(station);
            });
        }
    }

    return app.exec();
}
