#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../Common/ScenarioState.hpp"
#include "DashboardUiHelper.hpp"
#include "OperatorActionBox.hpp"
#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QString>

class QLabel;

namespace Ui {
class mainwindow;
}

class mainwindow : public QWidget
{
    Q_OBJECT

public:
    explicit mainwindow(QWidget *parent = nullptr );
    ~mainwindow();

    // Reads the latest telemetry rows and refreshes the operator widgets from the database.
    void RefreshTelemetryView();
    const char* LoadDatabase();

private:
    struct SatelliteRow
    {
        QString name;
        QString linkStatus;
        qint64 sequence = 0;
        qint64 receivedMs = 0;
        qint64 timestampMs = 0;
        double battery = 0.0;
        double temperature = 0.0;
    };

    // Copies the latest row for each satellite into the visible list.
    void PopulateSatelliteTable();
    // Copies recent telemetry rows for the selected satellite into the lower table.
    void PopulateTelemetryTable();
    // Pushes the currently selected satellite values into the detailed labels and Orbitview.
    void ApplySelectedSatellite();
    // Switches the operator UI into the "nothing is alive right now" state.
    void ShowNoTelemetryState(const QString& message);
    // Hides the detailed telemetry widgets until the operator has a satellite picked.
    void ShowNoSelectionState();
    // Brings the detailed widgets back once telemetry exists again.
    void ShowTelemetryState();
    // Handles row clicks in the satellite table and remembers which satellite the operator chose.
    void OnSatelliteRowClicked(int row, int column);
    // Finds the database file that actually has telemetry instead of an empty build copy.
    QString ResolveDatabasePath() const;
    // Button handler for kicking off the storm scenario.
    void OnTriggerSolarStorm();
    // Button handler for putting the scene back to normal.
    void OnResetScenario();
    // Button handler for sending a repair order for whichever satellite is selected.
    void OnRepairSelectedSatellite();
    // Drops a tiny operator command into SQLite so the sender can pick it up next loop.
    bool InsertRepairCommand(const QString& satelliteName);

    Ui::mainwindow *ui;
    QTimer* refreshTimer = nullptr;
    QVector<SatelliteRow> satellites;
    QString selectedSatelliteName;
    QString activeDatabasePath;
    QLabel* noSatelliteLabel = nullptr;
    // The storm/repair buttons and labels live in their own helper now so this class can breathe a little.
    OperatorActionBox operatorActionBox;
    bool stormIsActive = false;

};

#endif // MAINWINDOW_H
