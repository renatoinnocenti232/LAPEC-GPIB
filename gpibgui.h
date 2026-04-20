#ifndef GPIB_GUI_H
#define GPIB_GUI_H

#include <QMainWindow>
#include <QPushButton>
#include <QLCDNumber>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFutureWatcher>
#include <QTabWidget>
#include <QChartView>
#include <QLineSeries>
#include <QTimer>
#include <memory>

#include "gpib_config.h"
#include "measurementcontroller.h"
#include "scriptengine.h"
#include "websocketserver.h"

class GpibApp : public QMainWindow {
    Q_OBJECT

public:
    explicit GpibApp(QWidget *parent = nullptr);
    ~GpibApp();

private slots:
    // Slots da aba principal
    void aoClicarVarrer();
    void aoClicarConectar();
    void aoClicarLerIDN();
    void aoClicarStatusByte();
    void aoClicarEsperarSRQ();
    void aoClicarConfigurarEOS();
    void aoClicarMudarTimeout(int index);
    void aoClicarParallelPoll();
    void aoClicarIniciarLeitura();
    void aoClicarPararLeitura();
    void aoClicarExportarCSV();

    // Slots para resultados assíncronos
    void onEsperaSRQConcluida(bool ocorreu);
    void onNovaMedicao(double valor, qint64 timestamp);
    void onErroLeitura(const QString& erro);
    void onScriptOutput(const QString& texto);

    // Abas dinâmicas
    void onTabFechada(int index);

private:
    // ---------- Componentes da UI ----------
    QTabWidget *tabWidget;

    // Aba "Instrumento"
    QWidget *abaInstrumento;
    QPushButton *btnVarrer;
    QComboBox *comboEnderecos;
    QPushButton *btnConectar;
    QPushButton *btnLerIDN;
    QPushButton *btnStatusByte;
    QPushButton *btnEsperarSRQ;
    QPushButton *btnParallelPoll;
    QPushButton *btnIniciarLeitura;
    QPushButton *btnPararLeitura;
    QPushButton *btnExportarCSV;
    QTextEdit *logConsole;
    
    // Gráfico
    QChartView *chartView;
    QLineSeries *series;

    // Painel de configuração
    QComboBox *comboTimeout;
    QSpinBox *spinEOSChar;
    QCheckBox *checkTermLeitura;
    QCheckBox *checkEnviarEOI;
    QCheckBox *checkCmp8bits;
    QPushButton *btnAplicarEOS;

    // Aba "Scripts"
    QWidget *abaScript;
    QTextEdit *scriptEditor;
    QPushButton *btnExecutarScript;
    QPushButton *btnCarregarScript;
    QTextEdit *scriptOutput;

    // ---------- Estado interno ----------
    std::shared_ptr<Gpib::InstrumentoMestre> instrumentoAtual;
    int enderecoAtual = -1;
    QFutureWatcher<bool> futureWatcher;
    MeasurementController *measureController;
    ScriptEngine *scriptEngine;
    WebSocketServer *wsServer;

    // ---------- Métodos auxiliares ----------
    void configurarLayout();
    void criarAbaInstrumento();
    void criarAbaScript();
    void conectarInstrumento(int endereco, bool reset = true);
    void desconectarInstrumento();
    void logMensagem(const QString& msg, const QString& cor = "white");
    void aplicarConfiguracoesSalvas();
};

#endif // GPIB_GUI_H