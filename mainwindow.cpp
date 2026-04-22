#include "mainwindow.h"
#include "instrumenttab.h"
#include "scripttab.h"
#include "dashboardtab.h"
#include "hardware/measurementcontroller.h"
#include "automation/scriptengine.h"
#include "automation/commandhistory.h"
#include "automation/profilemanager.h"
#include "network/websocketserver.h"
#include "network/restapi.h"
#include "core/logger.h"
#include "core/appsettings.h"
#include "core/exceptionhandler.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    ExceptionHandler::instance().install();
    connect(&ExceptionHandler::instance(), &ExceptionHandler::unhandledException,
            this, &MainWindow::onUnhandledException);

    measureController = new MeasurementController(this);
    scriptEngine = new ScriptEngine(this);
    cmdHistory = new CommandHistory(this);
    profileManager = new ProfileManager(this);

    auto& sett = AppSettings::instance();
    bool apiEnabled = sett.apiHabilitada();
    bool localOnly = sett.apiApenasLocalhost();
    QString apiKey = sett.apiKey();

    wsServer = new WebSocketServer(sett.apiPortaWebSocket(), localOnly, this);
    wsServer->setApiKey(apiKey);
    if (apiEnabled) wsServer->iniciar();

    // RestApi seria similar (omitido por brevidade)

    setupUi();
    setupMenuBar();
    setupStatusBar();

    setWindowTitle(tr("GpibMaestro - Controle de Instrumentos GPIB"));
    resize(1000, 800);

    Logger::instance().info(tr("Interface principal inicializada."));
}

MainWindow::~MainWindow() {
    Logger::instance().info(tr("Interface principal destruída."));
}

void MainWindow::setupUi() {
    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    instrumentTab = new InstrumentTab(measureController, cmdHistory, profileManager, this);
    scriptTab = new ScriptTab(scriptEngine, this);
    dashboardTab = new DashboardTab(this);

    tabWidget->addTab(instrumentTab, tr("Instrumento"));
    tabWidget->addTab(scriptTab, tr("Scripts"));
    tabWidget->addTab(dashboardTab, tr("Dashboard"));

    connect(measureController, &MeasurementController::novaMedicao,
            dashboardTab, &DashboardTab::addDataPointToAll);
}

void MainWindow::setupMenuBar() {
    auto *fileMenu = menuBar()->addMenu(tr("&Arquivo"));
    auto *exitAction = fileMenu->addAction(tr("Sair"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto *helpMenu = menuBar()->addMenu(tr("&Ajuda"));
    auto *aboutAction = helpMenu->addAction(tr("Sobre"));
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, tr("Sobre GpibMaestro"),
            tr("GpibMaestro - Sistema de controle GPIB\nVersão 1.0"));
    });
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage(tr("Pronto"));
}

void MainWindow::onUnhandledException(const QString& msg, const QString& stack) {
    QMessageBox::critical(this, tr("Erro Fatal"),
        tr("Ocorreu um erro inesperado:\n%1\n\nStack trace:\n%2").arg(msg).arg(stack));
    Logger::instance().error("ERRO FATAL: " + msg);
}