#include <QApplication>
#include "mainwindow.h"
#include "core/logger.h"
#include "core/appsettings.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Configura logging
    Logger::instance().setLogFile("gpib_maestro.log");
    Logger::instance().info("Aplicação iniciada.");

    // Carrega configurações
    AppSettings::instance();

    MainWindow janela;
    janela.show();

    return app.exec();
}