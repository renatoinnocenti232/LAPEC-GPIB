/**
 * @file instrumenttab.h
 * @brief Aba principal de controle do instrumento.
 */

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QChartView>
#include <QLineSeries>
#include <QFutureWatcher>
#include <QQueue>
#include <memory>
#include "core/gpib_config.h"

class MeasurementController;
class CommandHistory;
class ProfileManager;

class InstrumentTab : public QWidget {
    Q_OBJECT

public:
    explicit InstrumentTab(MeasurementController *measureController,
                           CommandHistory *cmdHistory,
                           ProfileManager *profileManager,
                           QWidget *parent = nullptr);
    ~InstrumentTab();

signals:
    void logMessage(const QString& msg, const QString& color);

private slots:
    void onScanBus();
    void onConnect();
    void onDisconnect();
    void onReadIdn();
    void onReadStatusByte();
    void onWaitSrq();
    void onParallelPoll();
    void onApplyEos();
    void onChangeTimeout(int index);
    void onChangePlaca(int index);
    void onStartReading();
    void onStopReading();
    void onExportCsv();
    void onSendCommand();
    void onSaveProfile();
    void onLoadProfile();

    void onSrqFinished(bool occurred);
    void onMeasurementReceived(double value, qint64 timestamp);
    void onMeasurementError(const QString& error);

private:
    // UI setup
    void setupUi();
    void createControlBar(QHBoxLayout* controlLayout);
    void createChart();
    void createReadingControls(QHBoxLayout* readLayout);
    void createCommandLine(QHBoxLayout* cmdLayout);
    void createAdvancedSettings(QFormLayout* advancedLayout);
    
    // Helpers
    void applySavedSettings();
    void connectToInstrument(int address, int board, bool reset = true);
    void disconnectInstrument();
    void appendLog(const QString& msg, const QString& color = "white");
    void updateChartRange();
    void updateConnectionState(bool connected);

    // UI components
    QPushButton *btnScan;
    QComboBox *comboPlaca;
    QComboBox *comboAddress;
    QPushButton *btnConnect;
    QPushButton *btnDisconnect;
    QPushButton *btnIdn;
    QPushButton *btnStatus;
    QPushButton *btnSrq;
    QPushButton *btnPoll;
    QPushButton *btnStartRead;
    QPushButton *btnStopRead;
    QPushButton *btnExportCsv;
    QLineEdit *cmdLineEdit;
    QPushButton *btnSendCmd;
    QTextEdit *logConsole;

    QChartView *chartView;
    QLineSeries *series;
    QQueue<QPointF> dataBuffer;
    static const int MAX_BUFFER_SIZE = 500;

    QSpinBox *spinScanTimeout;
    QComboBox *comboTimeout;
    QSpinBox *spinEosChar;
    QCheckBox *checkEosTermRead;
    QCheckBox *checkEosSendEoi;
    QCheckBox *checkEos8bit;
    QPushButton *btnApplyEos;

    // State
    std::shared_ptr<Gpib::InstrumentoMestre> currentInstrument;
    int currentAddress = -1;
    int currentBoard = 0;
    QFutureWatcher<bool> srqWatcher;

    MeasurementController *measureController;
    CommandHistory *cmdHistory;
    ProfileManager *profileManager;
};