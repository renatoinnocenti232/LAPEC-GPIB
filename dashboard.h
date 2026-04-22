#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QTabWidget>
#include <QChartView>
#include <QLineSeries>

class Dashboard : public QWidget {
    Q_OBJECT
public:
    explicit Dashboard(QWidget *parent = nullptr);
    void addChart(const QString& title);
    void addDataPoint(int chartIndex, double value, qint64 timestamp);
    void clearChart(int chartIndex);

private:
    QTabWidget *tabWidget;
    QList<QLineSeries*> seriesList;
    QList<QChartView*> chartViews;
};

#endif