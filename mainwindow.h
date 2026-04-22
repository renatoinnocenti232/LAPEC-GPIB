/**
 * @file mainwindow.h
 * @brief Janela principal da aplicação.
 */

#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>

class InstrumentTab;
class ScriptTab;
class DashboardTab;
class MeasurementController;
class ScriptEngine;
class WebSocketServer;
class RestApi;
class ProfileManager;
class CommandHistory;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onUnhandledException(const QString& msg, const QString& stack);

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();

    QTabWidget *tabWidget;
    InstrumentTab *instrumentTab;
    ScriptTab *scriptTab;
    DashboardTab *dashboardTab;

    MeasurementController *measureController;
    ScriptEngine *scriptEngine;
    WebSocketServer *wsServer;
    RestApi *restApi;
    ProfileManager *profileManager;
    CommandHistory *cmdHistory;
};