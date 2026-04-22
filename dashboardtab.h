/**
 * @file dashboardtab.h
 * @brief Aba com múltiplos gráficos para visualização de dados.
 */

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QChartView>
#include <QLineSeries>

class DashboardTab : public QWidget {
    Q_OBJECT

public:
    explicit DashboardTab(QWidget *parent = nullptr);

    void addChart(const QString& title);
    void addDataPoint(int chartIndex, double value, qint64 timestamp);
    void clearChart(int chartIndex);

public slots:
    void addDataPointToAll(double value, qint64 timestamp);

private:
    QTabWidget *tabWidget;
    QList<QLineSeries*> seriesList;
    QList<QChartView*> chartViews;
};