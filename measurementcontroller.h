/**
 * @file measurementcontroller.h
 * @brief Controlador de leituras contínuas assíncronas.
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <QFutureWatcher>
#include <memory>
#include "gpib_config.h"

/**
 * @brief Gerencia leituras periódicas de um instrumento GPIB.
 * 
 * Utiliza QTimer e QFutureWatcher para evitar sobreposição de leituras.
 * Emite sinais com os valores medidos e erros.
 */
class MeasurementController : public QObject {
    Q_OBJECT
public:
    explicit MeasurementController(QObject *parent = nullptr);
    ~MeasurementController();

    /**
     * @brief Inicia leituras contínuas.
     * @param instr Instrumento a ser lido.
     * @param intervaloMs Intervalo desejado entre leituras.
     */
    void iniciarLeituraContinua(std::shared_ptr<Gpib::InstrumentoMestre> instr, int intervaloMs);

    /**
     * @brief Para as leituras.
     */
    void pararLeitura();

    bool estaLendo() const { return leituraAtiva_; }

signals:
    /**
     * @brief Emitido quando uma nova medição é obtida.
     * @param valor Valor numérico lido.
     * @param timestamp Timestamp em milissegundos desde a época.
     */
    void novaMedicao(double valor, qint64 timestamp);

    /**
     * @brief Emitido quando ocorre um erro durante a leitura.
     * @param erro Descrição do erro.
     */
    void erroLeitura(const QString& erro);

private slots:
    void realizarLeitura();

private:
    // CORREÇÃO: substituído QPointer por std::weak_ptr (InstrumentoMestre não é QObject)
    std::weak_ptr<Gpib::InstrumentoMestre> instrumento_;
    QFutureWatcher<double> futureWatcher_;
    QTimer timer_;                   // timer para leituras periódicas
    bool leituraAtiva_ = false;
};