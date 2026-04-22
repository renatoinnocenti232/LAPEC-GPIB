#include "instrumenttab.h"
#include "core/appsettings.h"
#include "core/logger.h"
#include <QValueAxis>

void InstrumentTab::applySavedSettings() {
    auto& sett = AppSettings::instance();
    int idx = comboTimeout->findData(static_cast<int>(sett.timeoutPadrao()));
    if (idx >= 0) comboTimeout->setCurrentIndex(idx);
    spinEosChar->setValue(sett.eosChar());
    checkEosTermRead->setChecked(sett.eosTermLeitura());
    checkEosSendEoi->setChecked(sett.eosEnviarEOI());
    checkEos8bit->setChecked(sett.eosCmp8bits());
    int placaIdx = comboPlaca->findData(sett.ultimaPlaca());
    if (placaIdx >= 0) comboPlaca->setCurrentIndex(placaIdx);
    int addrIdx = comboAddress->findData(sett.ultimoEndereco());
    if (addrIdx >= 0) comboAddress->setCurrentIndex(addrIdx);
}

void InstrumentTab::connectToInstrument(int address, int board, bool reset) {
    try {
        disconnectInstrument();
        currentInstrument = Gpib::GpibManager::instance().getInstrumento(address, board, reset);
        currentAddress = address;
        currentBoard = board;
        appendLog(tr("Conectado ao instrumento no endereço %1 (placa %2).").arg(address).arg(board), "lightgreen");
        AppSettings::instance().setUltimoEndereco(address);
        AppSettings::instance().setUltimaPlaca(board);
        updateConnectionState(true);
    } catch (const std::exception& e) {
        appendLog(tr("Falha ao conectar: %1").arg(e.what()), "red");
        currentInstrument.reset();
        currentAddress = -1;
        updateConnectionState(false);
    }
}

void InstrumentTab::disconnectInstrument() {
    if (currentInstrument) {
        Gpib::GpibManager::instance().removerInstrumento(currentAddress);
        currentInstrument.reset();
        currentAddress = -1;
        appendLog(tr("Instrumento desconectado."), "yellow");
        updateConnectionState(false);
    }
}

void InstrumentTab::appendLog(const QString& msg, const QString& color) {
    QString formatted = QString("<font color='%1'>%2</font>").arg(color).arg(msg);
    logConsole->append(formatted);
    Logger::instance().info(msg);
    emit logMessage(msg, color);
}

void InstrumentTab::updateChartRange() {
    if (series->count() > 0) {
        auto *axisX = qobject_cast<QValueAxis*>(chartView->chart()->axes(Qt::Horizontal).first());
        if (axisX) {
            axisX->setRange(series->at(0).x(), series->at(series->count()-1).x());
        }
    }
}

void InstrumentTab::updateConnectionState(bool connected) {
    btnDisconnect->setEnabled(connected);
    btnIdn->setEnabled(connected);
    btnStatus->setEnabled(connected);
    btnSrq->setEnabled(connected);
    btnPoll->setEnabled(connected);
    btnApplyEos->setEnabled(connected);
    btnSendCmd->setEnabled(connected);
    btnStartRead->setEnabled(connected && !measureController->estaLendo());
    btnStopRead->setEnabled(connected && measureController->estaLendo());
    comboTimeout->setEnabled(connected);
    spinEosChar->setEnabled(connected);
    checkEosTermRead->setEnabled(connected);
    checkEosSendEoi->setEnabled(connected);
    checkEos8bit->setEnabled(connected);
    if (!connected && measureController->estaLendo()) {
        measureController->pararLeitura();
        btnStartRead->setEnabled(false);
        btnStopRead->setEnabled(false);
    }
}