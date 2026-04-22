#include "dashboardtab.h"
#include <QVBoxLayout>
#include <QChart>
#include <QValueAxis>

DashboardTab::DashboardTab(QWidget *parent) : QWidget(parent) {
    tabWidget = new QTabWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
    addChart(tr("Canal 1"));
    addChart(tr("Canal 2"));
}

void DashboardTab::addChart(const QString& title) {
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

void DashboardTab::addDataPoint(int chartIndex, double value, qint64 timestamp) {
    if (chartIndex < 0 || chartIndex >= seriesList.size()) return;
    auto *series = seriesList[chartIndex];
    series->append(timestamp, value);
    if (series->count() > 500) {
        series->remove(0);
    }
    auto *chart = series->chart();
    if (chart && series->count() > 0) {
        auto *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
        if (axisX) {
            axisX->setRange(series->at(0).x(), series->at(series->count()-1).x());
        }
    }
}

void DashboardTab::clearChart(int chartIndex) {
    if (chartIndex >= 0 && chartIndex < seriesList.size())
        seriesList[chartIndex]->clear();
}

void DashboardTab::addDataPointToAll(double value, qint64 timestamp) {
    for (int i = 0; i < seriesList.size(); ++i) {
        addDataPoint(i, value, timestamp);
    }
}