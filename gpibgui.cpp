#include "gpib_gui.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>

#include "logger.h"
#include "appsettings.h"

GpibApp::GpibApp(QWidget *parent) : QMainWindow(parent) {
    // Inicializa módulos
    Logger::instance().setLogFile("gpib_maestro.log");
    Logger::instance().info("Aplicação iniciada.");
    
    measureController = new MeasurementController(this);
    scriptEngine = new ScriptEngine(this);
    wsServer = new WebSocketServer(12345, this);
    wsServer->iniciar();

    configurarLayout();
    aplicarConfiguracoesSalvas();

    connect(&futureWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
        if (futureWatcher.future().isValid()) {
            bool ocorreu = futureWatcher.result();
            onEsperaSRQConcluida(ocorreu);
        }
    });

    connect(measureController, &MeasurementController::novaMedicao,
            this, &GpibApp::onNovaMedicao);
    connect(measureController, &MeasurementController::erroLeitura,
            this, &GpibApp::onErroLeitura);
    connect(scriptEngine, &ScriptEngine::scriptOutput,
            this, &GpibApp::onScriptOutput);
}

GpibApp::~GpibApp() {
    futureWatcher.waitForFinished();
    measureController->pararLeitura();
    desconectarInstrumento();
    Logger::instance().info("Aplicação encerrada.");
}

void GpibApp::configurarLayout() {
    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    criarAbaInstrumento();
    criarAbaScript();

    setWindowTitle("GpibMaestro - Controle de Instrumentos GPIB");
    resize(900, 700);
}

void GpibApp::criarAbaInstrumento() {
    abaInstrumento = new QWidget();
    auto *layoutPrincipal = new QVBoxLayout(abaInstrumento);

    // Barra de controle superior
    auto *layoutControles = new QHBoxLayout();
    btnVarrer = new QPushButton("Escanear Barramento");
    comboEnderecos = new QComboBox();
    btnConectar = new QPushButton("Conectar");
    btnLerIDN = new QPushButton("Ler *IDN?");
    btnStatusByte = new QPushButton("Status Byte");
    btnEsperarSRQ = new QPushButton("Esperar SRQ");
    btnParallelPoll = new QPushButton("Parallel Poll");

    layoutControles->addWidget(btnVarrer);
    layoutControles->addWidget(new QLabel("Endereço:"));
    layoutControles->addWidget(comboEnderecos);
    layoutControles->addWidget(btnConectar);
    layoutControles->addWidget(btnLerIDN);
    layoutControles->addWidget(btnStatusByte);
    layoutControles->addWidget(btnEsperarSRQ);
    layoutControles->addWidget(btnParallelPoll);

    // Gráfico
    series = new QLineSeries();
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("Medições em tempo real");
    chartView = new QChartView(chart);
    chartView->setMinimumHeight(250);

    // Botões de leitura contínua
    auto *layoutLeitura = new QHBoxLayout();
    btnIniciarLeitura = new QPushButton("Iniciar Leitura Contínua");
    btnPararLeitura = new QPushButton("Parar Leitura");
    btnExportarCSV = new QPushButton("Exportar CSV");
    btnPararLeitura->setEnabled(false);
    layoutLeitura->addWidget(btnIniciarLeitura);
    layoutLeitura->addWidget(btnPararLeitura);
    layoutLeitura->addWidget(btnExportarCSV);

    // Console de log
    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet("background-color: #1e1e1e; color: #00ff00;");
    logConsole->setMaximumHeight(150);

    // Painel de configurações avançadas
    auto *grupoAvancado = new QGroupBox("Configurações Avançadas");
    auto *layoutAvancado = new QFormLayout(grupoAvancado);

    comboTimeout = new QComboBox();
    comboTimeout->addItem("10 µs", static_cast<int>(Gpib::Timeout::T10us));
    comboTimeout->addItem("100 ms", static_cast<int>(Gpib::Timeout::T100ms));
    comboTimeout->addItem("1 s", static_cast<int>(Gpib::Timeout::T1s));
    comboTimeout->addItem("10 s", static_cast<int>(Gpib::Timeout::T10s));
    comboTimeout->addItem("30 s", static_cast<int>(Gpib::Timeout::T30s));
    comboTimeout->setCurrentIndex(3);
    layoutAvancado->addRow("Timeout:", comboTimeout);

    spinEOSChar = new QSpinBox();
    spinEOSChar->setRange(0, 255);
    spinEOSChar->setValue(10);
    layoutAvancado->addRow("Caractere EOS:", spinEOSChar);

    checkTermLeitura = new QCheckBox("Terminar leitura no EOS");
    checkTermLeitura->setChecked(true);
    layoutAvancado->addRow(checkTermLeitura);

    checkEnviarEOI = new QCheckBox("Enviar EOI com EOS");
    layoutAvancado->addRow(checkEnviarEOI);

    checkCmp8bits = new QCheckBox("Comparação de 8 bits");
    layoutAvancado->addRow(checkCmp8bits);

    btnAplicarEOS = new QPushButton("Aplicar EOS");
    layoutAvancado->addRow(btnAplicarEOS);

    layoutPrincipal->addLayout(layoutControles);
    layoutPrincipal->addWidget(chartView);
    layoutPrincipal->addLayout(layoutLeitura);
    layoutPrincipal->addWidget(logConsole);
    layoutPrincipal->addWidget(grupoAvancado);

    tabWidget->addTab(abaInstrumento, "Instrumento");

    // Conexões de sinais
    connect(btnVarrer, &QPushButton::clicked, this, &GpibApp::aoClicarVarrer);
    connect(btnConectar, &QPushButton::clicked, this, &GpibApp::aoClicarConectar);
    connect(btnLerIDN, &QPushButton::clicked, this, &GpibApp::aoClicarLerIDN);
    connect(btnStatusByte, &QPushButton::clicked, this, &GpibApp::aoClicarStatusByte);
    connect(btnEsperarSRQ, &QPushButton::clicked, this, &GpibApp::aoClicarEsperarSRQ);
    connect(btnParallelPoll, &QPushButton::clicked, this, &GpibApp::aoClicarParallelPoll);
    connect(btnIniciarLeitura, &QPushButton::clicked, this, &GpibApp::aoClicarIniciarLeitura);
    connect(btnPararLeitura, &QPushButton::clicked, this, &GpibApp::aoClicarPararLeitura);
    connect(btnExportarCSV, &QPushButton::clicked, this, &GpibApp::aoClicarExportarCSV);
    connect(btnAplicarEOS, &QPushButton::clicked, this, &GpibApp::aoClicarConfigurarEOS);
    connect(comboTimeout, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GpibApp::aoClicarMudarTimeout);
}

void GpibApp::criarAbaScript() {
    abaScript = new QWidget();
    auto *layout = new QVBoxLayout(abaScript);

    scriptEditor = new QTextEdit();
    scriptEditor->setPlaceholderText("Digite seu script JavaScript aqui...\n"
                                     "Exemplo:\n"
                                     "gpib.log('Iniciando');\n"
                                     "var resp = gpib.consultar(5, '*IDN?');\n"
                                     "gpib.log('IDN: ' + resp);");

    auto *layoutBotoes = new QHBoxLayout();
    btnExecutarScript = new QPushButton("Executar");
    btnCarregarScript = new QPushButton("Carregar Arquivo...");
    layoutBotoes->addWidget(btnExecutarScript);
    layoutBotoes->addWidget(btnCarregarScript);
    layoutBotoes->addStretch();

    scriptOutput = new QTextEdit();
    scriptOutput->setReadOnly(true);
    scriptOutput->setStyleSheet("background-color: #f0f0f0;");

    layout->addWidget(new QLabel("Editor de Script:"));
    layout->addWidget(scriptEditor);
    layout->addLayout(layoutBotoes);
    layout->addWidget(new QLabel("Saída:"));
    layout->addWidget(scriptOutput);

    tabWidget->addTab(abaScript, "Scripts");

    connect(btnExecutarScript, &QPushButton::clicked, [this]() {
        scriptEngine->executarScript(scriptEditor->toPlainText());
    });
    connect(btnCarregarScript, &QPushButton::clicked, [this]() {
        QString caminho = QFileDialog::getOpenFileName(this, "Carregar Script", "", "Scripts (*.js);;Todos (*.*)");
        if (!caminho.isEmpty()) {
            QFile file(caminho);
            if (file.open(QIODevice::ReadOnly)) {
                scriptEditor->setPlainText(file.readAll());
            }
        }
    });
}

// ---------- Gerenciamento de conexão ----------
void GpibApp::conectarInstrumento(int endereco, bool reset) {
    try {
        desconectarInstrumento();
        instrumentoAtual = Gpib::GpibManager::instance().getInstrumento(endereco, 0, reset);
        enderecoAtual = endereco;
        logMensagem(QString("Conectado ao instrumento no endereço %1.").arg(endereco), "lightgreen");
        AppSettings::instance().setUltimoEndereco(endereco);
    } catch (const std::exception& e) {
        logMensagem(QString("Falha ao conectar: %1").arg(e.what()), "red");
        instrumentoAtual.reset();
        enderecoAtual = -1;
    }
}

void GpibApp::desconectarInstrumento() {
    if (instrumentoAtual) {
        Gpib::GpibManager::instance().removerInstrumento(enderecoAtual);
        instrumentoAtual.reset();
        enderecoAtual = -1;
        logMensagem("Instrumento desconectado.", "yellow");
    }
}

// ---------- Slots da aba Instrumento ----------
void GpibApp::aoClicarVarrer() {
    try {
        logMensagem("Iniciando busca de hardware...", "white");
        comboEnderecos->clear();
        desconectarInstrumento();

        auto lista = Gpib::InstrumentoMestre::VarrerBarramento(0);
        if (lista.empty()) {
            logMensagem("Nenhum dispositivo respondendo.", "yellow");
            return;
        }

        logMensagem(QString("%1 dispositivo(s) encontrado(s).").arg(lista.size()), "lightgreen");
        for (short addr : lista) {
            comboEnderecos->addItem(QString::number(addr), addr);
        }
        // Seleciona o último endereço usado (se existir)
        int ultimo = AppSettings::instance().ultimoEndereco();
        int idx = comboEnderecos->findData(ultimo);
        if (idx >= 0) comboEnderecos->setCurrentIndex(idx);
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
        QMessageBox::critical(this, "Erro", e.what());
    }
}

void GpibApp::aoClicarConectar() {
    if (comboEnderecos->count() == 0) return;
    int endereco = comboEnderecos->currentData().toInt();
    conectarInstrumento(endereco, true);
}

void GpibApp::aoClicarLerIDN() {
    if (!instrumentoAtual) {
        logMensagem("Nenhum instrumento conectado.", "red");
        return;
    }
    try {
        QString idn = QString::fromStdString(instrumentoAtual->query("*IDN?"));
        logMensagem(QString("<b>IDN [PAD %1]:</b> %2").arg(enderecoAtual).arg(idn), "white");
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
    }
}

void GpibApp::aoClicarStatusByte() {
    if (!instrumentoAtual) {
        logMensagem("Nenhum instrumento conectado.", "red");
        return;
    }
    try {
        uint8_t stb = instrumentoAtual->lerStatusByte();
        logMensagem(QString("[Status Byte] 0x%1 (%2)").arg(stb, 2, 16, QChar('0')).arg(stb), "white");
        if (stb & 0x40) {
            logMensagem("Dispositivo está requisitando serviço (RQS).", "orange");
        }
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
    }
}

void GpibApp::aoClicarEsperarSRQ() {
    if (!instrumentoAtual) {
        logMensagem("Nenhum instrumento conectado.", "red");
        return;
    }
    logMensagem("Aguardando SRQ em background...", "cyan");
    btnEsperarSRQ->setEnabled(false);
    auto future = QtConcurrent::run([this]() -> bool {
        try {
            return instrumentoAtual->esperarSRQ();
        } catch (...) {
            return false;
        }
    });
    futureWatcher.setFuture(future);
}

void GpibApp::onEsperaSRQConcluida(bool ocorreu) {
    btnEsperarSRQ->setEnabled(true);
    if (ocorreu) {
        logMensagem("SRQ detectado!", "lightgreen");
    } else {
        logMensagem("Timeout ou SRQ não ocorreu.", "yellow");
    }
}

void GpibApp::aoClicarParallelPoll() {
    try {
        uint8_t resultado = Gpib::InstrumentoMestre::executarParallelPoll(0);
        logMensagem(QString("[Parallel Poll] Resultado: 0x%1").arg(resultado, 2, 16, QChar('0')), "white");
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
    }
}

void GpibApp::aoClicarConfigurarEOS() {
    if (!instrumentoAtual) {
        logMensagem("Nenhum instrumento conectado.", "red");
        return;
    }
    try {
        uint8_t eos = static_cast<uint8_t>(spinEOSChar->value());
        bool termLeitura = checkTermLeitura->isChecked();
        bool enviarEOI = checkEnviarEOI->isChecked();
        bool cmp8 = checkCmp8bits->isChecked();
        instrumentoAtual->definirEOS(eos, termLeitura, enviarEOI, cmp8);
        logMensagem(QString("[EOS] Configurado: char=0x%1, termLeitura=%2, enviarEOI=%3, cmp8=%4")
                    .arg(eos, 2, 16).arg(termLeitura).arg(enviarEOI).arg(cmp8), "lightgreen");
        // Salva preferências
        AppSettings::instance().setEosChar(eos);
        AppSettings::instance().setEosTermLeitura(termLeitura);
        AppSettings::instance().setEosEnviarEOI(enviarEOI);
        AppSettings::instance().setEosCmp8bits(cmp8);
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
    }
}

void GpibApp::aoClicarMudarTimeout(int /*index*/) {
    if (!instrumentoAtual) return;
    try {
        Gpib::Timeout tmo = static_cast<Gpib::Timeout>(comboTimeout->currentData().toInt());
        instrumentoAtual->definirTimeout(tmo);
        AppSettings::instance().setTimeoutPadrao(tmo);
        logMensagem("Timeout alterado.", "lightgreen");
    } catch (const std::exception& e) {
        logMensagem(e.what(), "red");
    }
}

void GpibApp::aoClicarIniciarLeitura() {
    if (!instrumentoAtual) {
        logMensagem("Conecte um instrumento primeiro.", "red");
        return;
    }
    int intervalo = AppSettings::instance().intervaloLeituraMs();
    measureController->iniciarLeituraContinua(instrumentoAtual, intervalo);
    btnIniciarLeitura->setEnabled(false);
    btnPararLeitura->setEnabled(true);
    series->clear();
    logMensagem("Leitura contínua iniciada.", "cyan");
}

void GpibApp::aoClicarPararLeitura() {
    measureController->pararLeitura();
    btnIniciarLeitura->setEnabled(true);
    btnPararLeitura->setEnabled(false);
    logMensagem("Leitura contínua parada.", "cyan");
}

void GpibApp::onNovaMedicao(double valor, qint64 timestamp) {
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    series->append(dt.toMSecsSinceEpoch(), valor);
    // Mantém apenas os últimos 100 pontos para performance
    if (series->count() > 100)
        series->remove(0);
    chartView->chart()->axes(Qt::Horizontal).first()->setRange(
        series->at(0).x(), series->at(series->count()-1).x());
}

void GpibApp::onErroLeitura(const QString& erro) {
    logMensagem("Erro na leitura: " + erro, "red");
}

void GpibApp::aoClicarExportarCSV() {
    if (series->count() == 0) {
        logMensagem("Nenhum dado para exportar.", "yellow");
        return;
    }
    QString filename = QFileDialog::getSaveFileName(this, "Exportar CSV", "", "CSV (*.csv)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Timestamp,Valor\n";
        for (int i = 0; i < series->count(); ++i) {
            QPointF pt = series->at(i);
            out << QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(pt.x())).toString(Qt::ISODate)
                << "," << pt.y() << "\n";
        }
        logMensagem("Dados exportados com sucesso.", "lightgreen");
    } else {
        logMensagem("Falha ao criar arquivo.", "red");
    }
}

// ---------- Script output ----------
void GpibApp::onScriptOutput(const QString& texto) {
    scriptOutput->append(texto);
}

// ---------- Helpers ----------
void GpibApp::logMensagem(const QString& msg, const QString& cor) {
    QString formatado = QString("<font color='%1'>%2</font>").arg(cor).arg(msg);
    logConsole->append(formatado);
    Logger::instance().info(msg);
}

void GpibApp::aplicarConfiguracoesSalvas() {
    auto& sett = AppSettings::instance();
    int idx = comboTimeout->findData(static_cast<int>(sett.timeoutPadrao()));
    if (idx >= 0) comboTimeout->setCurrentIndex(idx);
    spinEOSChar->setValue(sett.eosChar());
    checkTermLeitura->setChecked(sett.eosTermLeitura());
    checkEnviarEOI->setChecked(sett.eosEnviarEOI());
    checkCmp8bits->setChecked(sett.eosCmp8bits());
}

void GpibApp::onTabFechada(int index) {
    // Nada necessário por enquanto
}