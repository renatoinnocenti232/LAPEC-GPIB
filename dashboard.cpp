#include "dashboard.h"
#include <QVBoxLayout>

Dashboard::Dashboard(QWidget *parent) : QWidget(parent) {
    tabWidget = new QTabWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
}

void Dashboard::addChart(const QString& title) {
    auto *series = new QLineSeries();
    seriesList.append(series);
    auto *chart = new QChart();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle(title);
    auto *view = new QChartView(chart);
    chartViews.append(view);
    tabWidget->addTab(view, title);
}

void Dashboard::addDataPoint(int chartIndex, double value, qint64 timestamp) {
    if (chartIndex < 0 || chartIndex >= seriesList.size()) return;
    seriesList[chartIndex]->append(timestamp, value);
    // Buffer circular: 500 pontos
    if (seriesList[chartIndex]->count() > 500)
        seriesList[chartIndex]->remove(0);
    auto *chart = seriesList[chartIndex]->chart();
    if (chart && seriesList[chartIndex]->count() > 0) {
        chart->axes(Qt::Horizontal).first()->setRange(
            seriesList[chartIndex]->at(0).x(),
            seriesList[chartIndex]->at(seriesList[chartIndex]->count()-1).x());
    }
}

void Dashboard::clearChart(int chartIndex) {
    if (chartIndex >= 0 && chartIndex < seriesList.size())
        seriesList[chartIndex]->clear();
}