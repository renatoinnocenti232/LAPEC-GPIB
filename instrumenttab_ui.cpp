#include "instrumenttab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QChart>
#include <QValueAxis>

void InstrumentTab::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);

    // Control bar
    auto *controlLayout = new QHBoxLayout();
    createControlBar(controlLayout);
    mainLayout->addLayout(controlLayout);

    // Chart
    createChart();
    mainLayout->addWidget(chartView);

    // Reading controls
    auto *readLayout = new QHBoxLayout();
    createReadingControls(readLayout);
    mainLayout->addLayout(readLayout);

    // Command line
    auto *cmdLayout = new QHBoxLayout();
    createCommandLine(cmdLayout);
    mainLayout->addLayout(cmdLayout);

    // Log console
    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet("background-color: #1e1e1e; color: #00ff00;");
    logConsole->setMaximumHeight(150);
    mainLayout->addWidget(logConsole);

    // Advanced settings
    auto *advancedGroup = new QGroupBox(tr("Configurações Avançadas"));
    auto *advancedLayout = new QFormLayout(advancedGroup);
    createAdvancedSettings(advancedLayout);
    mainLayout->addWidget(advancedGroup);
}

void InstrumentTab::createControlBar(QHBoxLayout* layout) {
    btnScan = new QPushButton(tr("Escanear"));
    comboPlaca = new QComboBox();
    for (int i = 0; i < 4; ++i) comboPlaca->addItem(tr("Placa %1").arg(i), i);
    comboAddress = new QComboBox();
    comboAddress->setEditable(true);
    comboAddress->setInsertPolicy(QComboBox::NoInsert);
    btnConnect = new QPushButton(tr("Conectar"));
    btnDisconnect = new QPushButton(tr("Desconectar"));
    btnIdn = new QPushButton(tr("*IDN?"));
    btnStatus = new QPushButton(tr("Status"));
    btnSrq = new QPushButton(tr("SRQ"));
    btnPoll = new QPushButton(tr("P.Poll"));

    layout->addWidget(btnScan);
    layout->addWidget(new QLabel(tr("Placa:")));
    layout->addWidget(comboPlaca);
    layout->addWidget(new QLabel(tr("End.:")));
    layout->addWidget(comboAddress);
    layout->addWidget(btnConnect);
    layout->addWidget(btnDisconnect);
    layout->addWidget(btnIdn);
    layout->addWidget(btnStatus);
    layout->addWidget(btnSrq);
    layout->addWidget(btnPoll);

    spinScanTimeout = new QSpinBox();
    spinScanTimeout->setRange(100, 30000);
    spinScanTimeout->setSuffix(" ms");
    spinScanTimeout->setValue(5000);
    spinScanTimeout->setToolTip(tr("Timeout para varredura do barramento"));
    layout->addWidget(new QLabel(tr("Timeout scan:")));
    layout->addWidget(spinScanTimeout);

    // Connections
    connect(btnScan, &QPushButton::clicked, this, &InstrumentTab::onScanBus);
    connect(btnConnect, &QPushButton::clicked, this, &InstrumentTab::onConnect);
    connect(btnDisconnect, &QPushButton::clicked, this, &InstrumentTab::onDisconnect);
    connect(btnIdn, &QPushButton::clicked, this, &InstrumentTab::onReadIdn);
    connect(btnStatus, &QPushButton::clicked, this, &InstrumentTab::onReadStatusByte);
    connect(btnSrq, &QPushButton::clicked, this, &InstrumentTab::onWaitSrq);
    connect(btnPoll, &QPushButton::clicked, this, &InstrumentTab::onParallelPoll);
    connect(comboPlaca, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InstrumentTab::onChangePlaca);
}

void InstrumentTab::createChart() {
    series = new QLineSeries();
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle(tr("Medições em tempo real"));
    chartView = new QChartView(chart);
    chartView->setMinimumHeight(250);
}

void InstrumentTab::createReadingControls(QHBoxLayout* layout) {
    btnStartRead = new QPushButton(tr("Iniciar Leitura"));
    btnStopRead = new QPushButton(tr("Parar"));
    btnExportCsv = new QPushButton(tr("Exportar CSV"));
    btnStopRead->setEnabled(false);
    layout->addWidget(btnStartRead);
    layout->addWidget(btnStopRead);
    layout->addWidget(btnExportCsv);

    connect(btnStartRead, &QPushButton::clicked, this, &InstrumentTab::onStartReading);
    connect(btnStopRead, &QPushButton::clicked, this, &InstrumentTab::onStopReading);
    connect(btnExportCsv, &QPushButton::clicked, this, &InstrumentTab::onExportCsv);
}

void InstrumentTab::createCommandLine(QHBoxLayout* layout) {
    cmdLineEdit = new QLineEdit();
    cmdLineEdit->setPlaceholderText(tr("Comando SCPI (ex: *IDN?)"));
    cmdLineEdit->setCompleter(cmdHistory->createCompleter(cmdLineEdit));
    btnSendCmd = new QPushButton(tr("Enviar"));
    layout->addWidget(cmdLineEdit);
    layout->addWidget(btnSendCmd);

    connect(btnSendCmd, &QPushButton::clicked, this, &InstrumentTab::onSendCommand);
}

void InstrumentTab::createAdvancedSettings(QFormLayout* layout) {
    comboTimeout = new QComboBox();
    comboTimeout->addItem("10 µs", static_cast<int>(Gpib::Timeout::T10us));
    comboTimeout->addItem("100 ms", static_cast<int>(Gpib::Timeout::T100ms));
    comboTimeout->addItem("1 s", static_cast<int>(Gpib::Timeout::T1s));
    comboTimeout->addItem("10 s", static_cast<int>(Gpib::Timeout::T10s));
    comboTimeout->addItem("30 s", static_cast<int>(Gpib::Timeout::T30s));
    comboTimeout->setCurrentIndex(3);
    layout->addRow(tr("Timeout:"), comboTimeout);

    spinEosChar = new QSpinBox();
    spinEosChar->setRange(0, 255);
    spinEosChar->setValue(10);
    layout->addRow(tr("Caractere EOS:"), spinEosChar);

    checkEosTermRead = new QCheckBox(tr("Terminar leitura no EOS"));
    checkEosTermRead->setChecked(true);
    layout->addRow(checkEosTermRead);

    checkEosSendEoi = new QCheckBox(tr("Enviar EOI com EOS"));
    layout->addRow(checkEosSendEoi);

    checkEos8bit = new QCheckBox(tr("Comparação de 8 bits"));
    layout->addRow(checkEos8bit);

    btnApplyEos = new QPushButton(tr("Aplicar EOS"));
    layout->addRow(btnApplyEos);

    auto *profileLayout = new QHBoxLayout();
    auto *btnSaveProfile = new QPushButton(tr("Salvar Perfil"));
    auto *btnLoadProfile = new QPushButton(tr("Carregar Perfil"));
    profileLayout->addWidget(btnSaveProfile);
    profileLayout->addWidget(btnLoadProfile);
    layout->addRow(profileLayout);

    connect(comboTimeout, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InstrumentTab::onChangeTimeout);
    connect(btnApplyEos, &QPushButton::clicked, this, &InstrumentTab::onApplyEos);
    connect(btnSaveProfile, &QPushButton::clicked, this, &InstrumentTab::onSaveProfile);
    connect(btnLoadProfile, &QPushButton::clicked, this, &InstrumentTab::onLoadProfile);
}