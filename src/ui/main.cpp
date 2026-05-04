#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "SynthController.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application properties
    app.setOrganizationName("TMM88");
    app.setApplicationName("TMM88 Synth");
    app.setApplicationVersion("1.0");

    // Create the synth controller
    SynthController synthController;

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Expose the synth controller to QML
    engine.rootContext()->setContextProperty("synthController", &synthController);

    // Load the main QML file
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}