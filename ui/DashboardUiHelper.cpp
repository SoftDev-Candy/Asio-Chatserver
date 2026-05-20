#include "DashboardUiHelper.hpp"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QTableWidget>
#include <chrono>

#include "ui_mainwindow.h"

qint64 DashboardUiHelper::NowMs()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

QString DashboardUiHelper::LinkStatus(qint64 ageMs)
{
    if (ageMs < 3000)
    {
        return "Connected";
    }

    if (ageMs <= 7000)
    {
        return "Degraded";
    }

    return "Disconnected";
}

QString DashboardUiHelper::BatteryStatus(double battery)
{
    if (battery < 30.0)
    {
        return "Critical";
    }

    if (battery <= 70.0)
    {
        return "Normal";
    }

    return "Good";
}

QString DashboardUiHelper::TemperatureStatus(double temperature)
{
    if (temperature < 0.0)
    {
        return "Critical Low";
    }

    if (temperature < 15.0)
    {
        return "Low";
    }

    if (temperature <= 60.0)
    {
        return "Normal";
    }

    if (temperature <= 75.0)
    {
        return "High";
    }

    return "Critical High";
}

QString DashboardUiHelper::HealthStatus(const QString& linkStatus, const QString& batteryStatus, const QString& temperatureStatus)
{
    if (linkStatus == "Disconnected")
    {
        return "Offline";
    }

    if (batteryStatus.contains("Critical") || temperatureStatus.contains("Critical"))
    {
        return "Attention";
    }

    if (linkStatus == "Degraded" || temperatureStatus == "High")
    {
        return "Warning";
    }

    return "Nominal";
}

QString DashboardUiHelper::FormatTime(qint64 epochMs)
{
    return QDateTime::fromMSecsSinceEpoch(epochMs).toString("yyyy-MM-dd hh:mm:ss");
}

QString DashboardUiHelper::EstimatePacketSize(const QString& satName, qint64 sequence, qint64 timestampMs, double battery, double temperature)
{
    const QString payload = QString("{\"sat_id\":\"%1\",\"sequence\":%2,\"timestamp_ms\":%3,\"battery\":%4,\"temp_c\":%5}")
        .arg(satName)
        .arg(sequence)
        .arg(timestampMs)
        .arg(QString::number(battery, 'f', 2))
        .arg(QString::number(temperature, 'f', 2));

    return QString::number(payload.toUtf8().size()) + " B";
}

void DashboardUiHelper::ConfigureSatelliteTable(QTableWidget* table)
{
    if (table == nullptr)
    {
        return;
    }

    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({
        "Satellite Name",
        "Link Status",
        "Battery",
        "Temperature"
    });
    table->setRowCount(0);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void DashboardUiHelper::ConfigureTelemetryTable(QTableWidget* table)
{
    if (table == nullptr)
    {
        return;
    }

    table->setColumnCount(10);
    table->setHorizontalHeaderLabels({
        "Sequence",
        "Timesent_ms",
        "Received_ms",
        "Latency_ms",
        "Age_ms",
        "Link Status",
        "Battery",
        "Temperature",
        "PacketSize",
        "Date & Time"
    });
    table->setRowCount(0);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setDefaultSectionSize(120);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setCursor(Qt::ArrowCursor);
}

void DashboardUiHelper::StyleDashboard(Ui::mainwindow* ui)
{
    if (ui == nullptr)
    {
        return;
    }

    const QString tableStyle =
        "QTableWidget {"
        " background-color: rgba(8, 14, 24, 210);"
        " color: rgb(232, 238, 245);"
        " gridline-color: rgba(120, 150, 180, 70);"
        " border: 1px solid rgba(120, 150, 180, 90);"
        " border-radius: 10px;"
        " selection-background-color: rgba(72, 145, 220, 160);"
        " selection-color: white;"
        "}"
        "QHeaderView::section {"
        " background-color: rgba(20, 30, 46, 220);"
        " color: rgb(226, 234, 244);"
        " border: none;"
        " padding: 6px;"
        " font-weight: 600;"
        "}";

    ui->SatelliteTable->setStyleSheet(tableStyle);
    ui->DatabaseTable->setStyleSheet(tableStyle);

    ui->layoutWidget->setAttribute(Qt::WA_StyledBackground, true);
    ui->layoutWidget_2->setAttribute(Qt::WA_StyledBackground, true);

    const QString panelStyle =
        "QWidget {"
        " background-color: rgba(8, 14, 24, 170);"
        " border: 1px solid rgba(120, 150, 180, 90);"
        " border-radius: 12px;"
        "}"
        "QLabel {"
        " color: rgb(232, 238, 245);"
        " background: transparent;"
        " border: none;"
        "}";

    ui->layoutWidget->setStyleSheet(panelStyle);
    ui->layoutWidget_2->setStyleSheet(panelStyle);

    ui->SatelliteData_Left->setContentsMargins(14, 14, 14, 14);
    ui->SatelliteData_Right->setContentsMargins(14, 14, 14, 14);
    ui->SatelliteData_Left->setSpacing(10);
    ui->SatelliteData_Right->setSpacing(12);
    ui->SatelliteData_Left->setAlignment(Qt::AlignTop);
    ui->SatelliteData_Right->setAlignment(Qt::AlignTop);

    ui->SatelliteName_Layout->setAlignment(Qt::AlignLeft);
    ui->LinkStatus_Layout->setAlignment(Qt::AlignLeft);
    ui->PacketLoss_Layout->setAlignment(Qt::AlignLeft);
    ui->Latency_Layout->setAlignment(Qt::AlignLeft);
    ui->Battery_Layout->setAlignment(Qt::AlignLeft);
    ui->Temprature_Layout->setAlignment(Qt::AlignLeft);
    ui->Battery_StatusLayout->setAlignment(Qt::AlignLeft);
    ui->Health_StatusLayout->setAlignment(Qt::AlignLeft);
    ui->Temp_StatusLayout->setAlignment(Qt::AlignLeft);

    ui->SatelliteName_Header->setMinimumWidth(96);
    ui->LinkStatus_Header->setMinimumWidth(96);
    ui->PacketLoss_Header->setMinimumWidth(96);
    ui->Latency_Header->setMinimumWidth(96);
    ui->Battery_Header->setMinimumWidth(96);
    ui->Latency_Header_3->setMinimumWidth(96);
    ui->Battery_StatusHeader->setMinimumWidth(108);
    ui->Health_StatusHeader->setMinimumWidth(108);
    ui->Temp_StatusHeader->setMinimumWidth(108);

    ui->SatelliteName_Label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->LinkStatus_Connection->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->PacketLoss_Connection->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Latency_Data->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Battery_Data->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Temprature_Data->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Battery_StatusData->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Health_StatusData->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->Temp_StatusData->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    ui->Battery_StatusHeader->setText("Battery :");
    ui->Health_StatusHeader->setText("Health :");
    ui->Temp_StatusHeader->setText("Temp :");
}

void DashboardUiHelper::ClearDetailLabels(Ui::mainwindow* ui)
{
    if (ui == nullptr)
    {
        return;
    }

    ui->SatelliteName_Label->setText("N/A");
    ui->LinkStatus_Connection->setText("Disconnected");
    ui->PacketLoss_Connection->setText("N/A");
    ui->Latency_Data->setText("N/A");
    ui->Battery_Data->setText("N/A");
    ui->Temprature_Data->setText("N/A");
    ui->Battery_StatusData->setText("N/A");
    ui->Temp_StatusData->setText("N/A");
    ui->Health_StatusData->setText("N/A");
}
