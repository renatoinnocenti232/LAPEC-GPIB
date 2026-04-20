#include "measurementcontroller.h"
#include <QtConcurrent/QtConcurrent>
#include "logger.h"

MeasurementController::MeasurementController(QObject *parent) : QObject(parent) {
    connect(&timer_, &QTimer::timeout, this, &MeasurementController::realizarLeitura);
    connect(&futureWatcher_, &QFutureWatcher<double>::finished, this, [this]() {
        try {
            double valor = futureWatcher_.result();
            emit novaMedicao(valor, QDateTime::currentMSecsSinceEpoch());
        } catch (const std::exception& e) {
            emit erroLeitura(e.what());
        }
    });
}

MeasurementController::~MeasurementController() {
    pararLeitura();
}

void MeasurementController::iniciarLeituraContinua(std::shared_ptr<Gpib::InstrumentoMestre> instr, int intervaloMs) {
    if (leituraAtiva_) pararLeitura();
    instrumento_ = instr;
    leituraAtiva_ = true;
    timer_.start(intervaloMs);
    Logger::instance().info("Leitura contínua iniciada.");
}

void MeasurementController::pararLeitura() {
    if (leituraAtiva_) {
        timer_.stop();
        futureWatcher_.waitForFinished();
        leituraAtiva_ = false;
        Logger::instance().info("Leitura contínua parada.");
    }
}

void MeasurementController::realizarLeitura() {
    if (!instrumento_ || futureWatcher_.isRunning()) return;

    auto future = QtConcurrent::run([this]() -> double {
        // Exemplo: ler valor numérico (assume que instrumento retorna string com número)
        std::string resp = instrumento_->query("READ?");
        return std::stod(resp);
    });
    futureWatcher_.setFuture(future);
}