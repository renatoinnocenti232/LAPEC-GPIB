#include <QApplication>
#include "gpib_gui.h"
#include "logger.h"
#include "appsettings.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Configura logging
    Logger::instance().setLogFile("gpib_maestro.log");
    Logger::instance().info("Aplicação iniciada.");

    // Carrega configurações
    AppSettings::instance();

    GpibApp janela;
    janela.show();

    return app.exec();
}