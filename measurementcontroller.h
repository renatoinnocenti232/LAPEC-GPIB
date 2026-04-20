#ifndef MEASUREMENTCONTROLLER_H
#define MEASUREMENTCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QFutureWatcher>
#include <QtCharts/QLineSeries>
#include "gpib_config.h"

class MeasurementController : public QObject {
    Q_OBJECT
public:
    explicit MeasurementController(QObject *parent = nullptr);
    ~MeasurementController();

    void iniciarLeituraContinua(std::shared_ptr<Gpib::InstrumentoMestre> instr, int intervaloMs);
    void pararLeitura();
    bool estaLendo() const { return leituraAtiva_; }

signals:
    void novaMedicao(double valor, qint64 timestamp);
    void erroLeitura(const QString& erro);

private slots:
    void realizarLeitura();

private:
    QTimer timer_;
    std::shared_ptr<Gpib::InstrumentoMestre> instrumento_;
    QFutureWatcher<double> futureWatcher_;
    bool leituraAtiva_ = false;
};

#endif