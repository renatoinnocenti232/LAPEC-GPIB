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
        } catch (...) {
            emit erroLeitura(tr("Erro desconhecido na leitura."));
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
        std::string resp = instrumento_->query("READ?");
        try {
            return std::stod(resp);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Resposta não numérica: " + resp);
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Valor fora do intervalo: " + resp);
        }
    });
    futureWatcher_.setFuture(future);
}#include "measurementcontroller.h"
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
        } catch (...) {
            emit erroLeitura(tr("Erro desconhecido na leitura."));
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
    auto instr = instrumento_.lock();
    if (!instr || futureWatcher_.isRunning()) return;

    auto future = QtConcurrent::run([instr]() -> double {
        std::string resp = instr->query("READ?");
        try {
            return std::stod(resp);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Resposta não numérica: " + resp);
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Valor fora do intervalo: " + resp);
        }
    });
    futureWatcher_.setFuture(future);
}