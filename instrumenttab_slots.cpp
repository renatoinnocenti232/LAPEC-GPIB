#include "instrumenttab.h"
#include "hardware/measurementcontroller.h"
#include "automation/commandhistory.h"
#include "automation/profilemanager.h"
#include "core/logger.h"
#include "core/appsettings.h"
#include "core/threadpool.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QtConcurrent/QtConcurrent>

InstrumentTab::InstrumentTab(MeasurementController *measureCtrl,
                             CommandHistory *cmdHist,
                             ProfileManager *profMgr,
                             QWidget *parent)
    : QWidget(parent)
    , measureController(measureCtrl)
    , cmdHistory(cmdHist)
    , profileManager(profMgr) {
    setupUi();
    applySavedSettings();
    updateConnectionState(false);

    connect(&srqWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
        if (srqWatcher.future().isValid())
            onSrqFinished(srqWatcher.result());
    });

    connect(measureController, &MeasurementController::novaMedicao,
            this, &InstrumentTab::onMeasurementReceived);
    connect(measureController, &MeasurementController::erroLeitura,
            this, &InstrumentTab::onMeasurementError);
}

InstrumentTab::~InstrumentTab() {
    disconnectInstrument();
}

// Implementações dos slots

void InstrumentTab::onScanBus() {
    int board = comboPlaca->currentData().toInt();
    int timeoutMs = spinScanTimeout->value();
    appendLog(tr("Iniciando busca na placa %1 (timeout %2 ms)...").arg(board).arg(timeoutMs), "white");

    GpibThreadPool::instance().run([this, board, timeoutMs]() {
        try {
            auto lista = Gpib::InstrumentoMestre::VarrerBarramento(board, timeoutMs);
            QMetaObject::invokeMethod(this, [this, lista]() {
                comboAddress->clear();
                if (lista.empty()) {
                    appendLog(tr("Nenhum dispositivo respondendo."), "yellow");
                    return;
                }
                appendLog(tr("%1 dispositivo(s) encontrado(s).").arg(lista.size()), "lightgreen");
                for (short addr : lista) {
                    comboAddress->addItem(QString::number(addr), addr);
                }
                int ultimo = AppSettings::instance().ultimoEndereco();
                int idx = comboAddress->findData(ultimo);
                if (idx >= 0) comboAddress->setCurrentIndex(idx);
            });
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(this, [this, e]() {
                comboAddress->clear(); // MELHORIA
                appendLog(e.what(), "red");
                QMessageBox::critical(this, tr("Erro"), e.what());
            });
        }
    });
}

void InstrumentTab::onConnect() {
    int board = comboPlaca->currentData().toInt();
    int address = comboAddress->currentData().toInt();
    if (address == 0) {
        bool ok;
        address = QInputDialog::getInt(this, tr("Endereço GPIB"), tr("Digite o endereço (0-30):"), 1, 0, 30, 1, &ok);
        if (!ok) return;
    }
    connectToInstrument(address, board, true);
}

void InstrumentTab::onDisconnect() {
    disconnectInstrument();
}

void InstrumentTab::onReadIdn() {
    if (!currentInstrument) {
        appendLog(tr("Nenhum instrumento conectado."), "red");
        return;
    }
    GpibThreadPool::instance().run([this]() {
        try {
            std::string idn = currentInstrument->query("*IDN?");
            QMetaObject::invokeMethod(this, [this, idn]() {
                appendLog(QString("<b>IDN [PAD %1]:</b> %2").arg(currentAddress).arg(QString::fromStdString(idn)), "white");
            });
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(this, [this, e]() {
                appendLog(e.what(), "red");
            });
        }
    });
}

void InstrumentTab::onReadStatusByte() {
    if (!currentInstrument) {
        appendLog(tr("Nenhum instrumento conectado."), "red");
        return;
    }
    try {
        uint8_t stb = currentInstrument->lerStatusByte();
        appendLog(QString("[Status Byte] 0x%1 (%2)").arg(stb, 2, 16, QChar('0')).arg(stb), "white");
        if (stb & 0x40) {
            appendLog(tr("Dispositivo está requisitando serviço (RQS)."), "orange");
        }
    } catch (const std::exception& e) {
        appendLog(e.what(), "red");
    }
}

void InstrumentTab::onWaitSrq() {
    if (!currentInstrument) {
        appendLog(tr("Nenhum instrumento conectado."), "red");
        return;
    }
    appendLog(tr("Aguardando SRQ em background..."), "cyan");
    btnSrq->setEnabled(false);
    auto future = GpibThreadPool::instance().run([this]() -> bool {
        try {
            return currentInstrument->esperarSRQ();
        } catch (...) {
            return false;
        }
    });
    srqWatcher.setFuture(future);
}

void InstrumentTab::onSrqFinished(bool occurred) {
    btnSrq->setEnabled(true);
    if (occurred) {
        appendLog(tr("SRQ detectado!"), "lightgreen");
    } else {
        appendLog(tr("Timeout ou SRQ não ocorreu."), "yellow");
    }
}

void InstrumentTab::onParallelPoll() {
    try {
        int board = comboPlaca->currentData().toInt();
        uint8_t result = Gpib::InstrumentoMestre::executarParallelPoll(board);
        appendLog(QString("[Parallel Poll] Resultado: 0x%1").arg(result, 2, 16, QChar('0')), "white");
    } catch (const std::exception& e) {
        appendLog(e.what(), "red");
    }
}

void InstrumentTab::onApplyEos() {
    if (!currentInstrument) {
        appendLog(tr("Nenhum instrumento conectado."), "red");
        return;
    }
    try {
        uint8_t eos = static_cast<uint8_t>(spinEosChar->value());
        bool termRead = checkEosTermRead->isChecked();
        bool sendEoi = checkEosSendEoi->isChecked();
        bool cmp8 = checkEos8bit->isChecked();
        currentInstrument->definirEOS(eos, termRead, sendEoi, cmp8);
        appendLog(QString("[EOS] Configurado: char=0x%1, termLeitura=%2, enviarEOI=%3, cmp8=%4")
                  .arg(eos, 2, 16).arg(termRead).arg(sendEoi).arg(cmp8), "lightgreen");
        auto& sett = AppSettings::instance();
        sett.setEosChar(eos);
        sett.setEosTermLeitura(termRead);
        sett.setEosEnviarEOI(sendEoi);
        sett.setEosCmp8bits(cmp8);
    } catch (const std::exception& e) {
        appendLog(e.what(), "red");
    }
}

void InstrumentTab::onChangeTimeout(int /*index*/) {
    if (!currentInstrument) return;
    try {
        Gpib::Timeout tmo = static_cast<Gpib::Timeout>(comboTimeout->currentData().toInt());
        currentInstrument->definirTimeout(tmo);
        AppSettings::instance().setTimeoutPadrao(tmo);
        appendLog(tr("Timeout alterado."), "lightgreen");
    } catch (const std::exception& e) {
        appendLog(e.what(), "red");
    }
}

void InstrumentTab::onChangePlaca(int /*index*/) {
    AppSettings::instance().setUltimaPlaca(comboPlaca->currentData().toInt());
}

void InstrumentTab::onStartReading() {
    if (!currentInstrument) {
        appendLog(tr("Conecte um instrumento primeiro."), "red");
        return;
    }
    int interval = AppSettings::instance().intervaloLeituraMs();
    measureController->iniciarLeituraContinua(currentInstrument, interval);
    btnStartRead->setEnabled(false);
    btnStopRead->setEnabled(true);
    series->clear();
    dataBuffer.clear();
    appendLog(tr("Leitura contínua iniciada."), "cyan");
}

void InstrumentTab::onStopReading() {
    measureController->pararLeitura();
    btnStartRead->setEnabled(true);
    btnStopRead->setEnabled(false);
    appendLog(tr("Leitura contínua parada."), "cyan");
}

void InstrumentTab::onMeasurementReceived(double value, qint64 timestamp) {
    QPointF point(timestamp, value);
    dataBuffer.enqueue(point);
    if (dataBuffer.size() > MAX_BUFFER_SIZE) {
        dataBuffer.dequeue();
    }
    series->replace(dataBuffer.toList());
    updateChartRange();
}

void InstrumentTab::onMeasurementError(const QString& error) {
    appendLog(tr("Erro na leitura: ") + error, "red");
}

void InstrumentTab::onExportCsv() {
    if (series->count() == 0) {
        appendLog(tr("Nenhum dado para exportar."), "yellow");
        return;
    }
    QString filename = QFileDialog::getSaveFileName(this, tr("Exportar CSV"), "", "CSV (*.csv)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        bool withHeader = AppSettings::instance().exportarCSVComCabecalho();
        if (withHeader) out << "Timestamp,Valor\n";
        for (int i = 0; i < series->count(); ++i) {
            QPointF pt = series->at(i);
            out << QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(pt.x())).toString(Qt::ISODate)
                << "," << pt.y() << "\n";
        }
        appendLog(tr("Dados exportados com sucesso."), "lightgreen");
    } else {
        appendLog(tr("Falha ao criar arquivo."), "red");
    }
}

void InstrumentTab::onSendCommand() {
    if (!currentInstrument) {
        appendLog(tr("Conecte um instrumento primeiro."), "red");
        return;
    }
    QString cmd = cmdLineEdit->text().trimmed();
    if (cmd.isEmpty()) return;
    cmdHistory->addCommand(cmd);
    GpibThreadPool::instance().run([this, cmd]() {
        try {
            if (cmd.endsWith('?')) {
                std::string resp = currentInstrument->query(cmd.toStdString());
                QMetaObject::invokeMethod(this, [this, cmd, resp]() {
                    appendLog(QString("> %1\n%2").arg(cmd).arg(QString::fromStdString(resp)), "lightblue");
                });
            } else {
                currentInstrument->enviar(cmd.toStdString());
                QMetaObject::invokeMethod(this, [this, cmd]() {
                    appendLog(QString("> %1 (ok)").arg(cmd), "lightblue");
                });
            }
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(this, [this, e]() {
                appendLog(tr("Erro: %1").arg(e.what()), "red");
            });
        }
    });
    cmdLineEdit->clear();
}

void InstrumentTab::onSaveProfile() {
    QString name = QInputDialog::getText(this, tr("Salvar Perfil"), tr("Nome do perfil:"));
    if (name.isEmpty()) return;
    QVariantMap map;
    map["timeout"] = comboTimeout->currentData().toInt();
    map["eosChar"] = spinEosChar->value();
    map["termLeitura"] = checkEosTermRead->isChecked();
    map["enviarEOI"] = checkEosSendEoi->isChecked();
    map["cmp8bits"] = checkEos8bit->isChecked();
    map["ultimoEndereco"] = comboAddress->currentData().toInt();
    map["ultimaPlaca"] = comboPlaca->currentData().toInt();
    map["scanTimeout"] = spinScanTimeout->value();
    if (profileManager->saveProfile(name, map))
        appendLog(tr("Perfil salvo com sucesso."), "lightgreen");
    else
        appendLog(tr("Falha ao salvar perfil."), "red");
}

void InstrumentTab::onLoadProfile() {
    QStringList profiles = profileManager->listProfiles();
    if (profiles.isEmpty()) {
        appendLog(tr("Nenhum perfil disponível."), "yellow");
        return;
    }
    QString name = QInputDialog::getItem(this, tr("Carregar Perfil"), tr("Selecione o perfil:"), profiles, 0, false);
    if (name.isEmpty()) return;
    QVariantMap map = profileManager->loadProfile(name);
    if (map.isEmpty()) {
        appendLog(tr("Falha ao carregar perfil."), "red");
        return;
    }
    int idx = comboTimeout->findData(map["timeout"].toInt());
    if (idx >= 0) comboTimeout->setCurrentIndex(idx);
    spinEosChar->setValue(map["eosChar"].toInt());
    checkEosTermRead->setChecked(map["termLeitura"].toBool());
    checkEosSendEoi->setChecked(map["enviarEOI"].toBool());
    checkEos8bit->setChecked(map["cmp8bits"].toBool());
    int addr = map["ultimoEndereco"].toInt();
    idx = comboAddress->findData(addr);
    if (idx >= 0) comboAddress->setCurrentIndex(idx);
    int placa = map["ultimaPlaca"].toInt();
    idx = comboPlaca->findData(placa);
    if (idx >= 0) comboPlaca->setCurrentIndex(idx);
    spinScanTimeout->setValue(map["scanTimeout"].toInt());
    appendLog(tr("Perfil carregado com sucesso."), "lightgreen");
}