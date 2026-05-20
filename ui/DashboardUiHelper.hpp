#ifndef DASHBOARDUIHELPER_HPP
#define DASHBOARDUIHELPER_HPP

#include <QString>
#include <QtGlobal>

class QTableWidget;

namespace Ui
{
class mainwindow;
}

// Small UI helper for the dashboard widgets.
// Keeps the repetitive table/panel setup out of MainWindow so that file stays easier to scan.
class DashboardUiHelper
{
public:
    static qint64 NowMs();
    static QString LinkStatus(qint64 ageMs);
    static QString BatteryStatus(double battery);
    static QString TemperatureStatus(double temperature);
    static QString HealthStatus(const QString& linkStatus, const QString& batteryStatus, const QString& temperatureStatus);
    static QString FormatTime(qint64 epochMs);
    static QString EstimatePacketSize(const QString& satName, qint64 sequence, qint64 timestampMs, double battery, double temperature);

    static void ConfigureSatelliteTable(QTableWidget* table);
    static void ConfigureTelemetryTable(QTableWidget* table);
    static void StyleDashboard(Ui::mainwindow* ui);
    static void ClearDetailLabels(Ui::mainwindow* ui);
};

#endif // DASHBOARDUIHELPER_HPP
