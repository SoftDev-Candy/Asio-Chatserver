#include "mainwindow.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidgetItem>
#include <QStringList>
#include <iostream>
#include "ui_mainwindow.h"
#include "../external/sqlite/sqlite3.h"

mainwindow::mainwindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainwindow)
{
    ui->setupUi(this);
    resize(1280, 860);
    setMinimumSize(1280, 860);

    // Basic widget setup gets pushed into a helper so this constructor stays readable (❁´◡`❁)
    DashboardUiHelper::ConfigureTelemetryTable(ui->DatabaseTable);
    DashboardUiHelper::ConfigureSatelliteTable(ui->SatelliteTable);
    DashboardUiHelper::StyleDashboard(ui);
    operatorActionBox.Build(this);

    // If the UI starts while the storm scenario file already says "storm on", match that right away.
    ScenarioState sharedScenario = LoadSharedScenarioState();
    stormIsActive = sharedScenario == ScenarioState::SolarStormActive;
    operatorActionBox.SetStormWarningVisible(sharedScenario == ScenarioState::SolarStormActive);
    if (ui->OrbitView != nullptr)
    {
        // Sync the scene to the shared scenario file right away so the UI and sender do not start out mismatched (￣y▽,￣)╭
        ui->OrbitView->SetScenarioState(sharedScenario);
    }

    // This label is our little "yo... nothing is talking yet" message (┬┬﹏┬┬)
    noSatelliteLabel = new QLabel("No active satellite connection", this);
    noSatelliteLabel->setGeometry(16, 20, 260, 24);
    noSatelliteLabel->setStyleSheet("color: rgb(235, 235, 235); font-weight: 600;");
    noSatelliteLabel->hide();

    // Clicking a satellite row is what wakes up the detail panel and the 3D selection. Main character moment fr.
    connect(ui->SatelliteTable, &QTableWidget::cellClicked, this, &mainwindow::OnSatelliteRowClicked);
    connect(operatorActionBox.StormButton(), &QPushButton::clicked, this, &mainwindow::OnTriggerSolarStorm);
    connect(operatorActionBox.ResetButton(), &QPushButton::clicked, this, &mainwindow::OnResetScenario);
    connect(operatorActionBox.RepairButton(), &QPushButton::clicked, this, &mainwindow::OnRepairSelectedSatellite);

    // Refresh once a second so the dashboard keeps up with new rows without overworking the UI. Nice and chill ¯\_(ツ)_/¯
    refreshTimer = new QTimer(this);
    connect(refreshTimer , &QTimer::timeout ,this , &mainwindow::RefreshTelemetryView);

    RefreshTelemetryView();
    refreshTimer->start(1000);
}

mainwindow::~mainwindow()
{
    delete ui;
}

void mainwindow::OnTriggerSolarStorm()
{
    operatorActionBox.SetStormWarningVisible(true);
    SaveSharedScenarioState(ScenarioState::SolarStormActive);
    stormIsActive = true;

    if (ui->OrbitView != nullptr)
    {
        // No OrbitView means maybe the UI is only half alive, so we quietly skip the visual push here. No drama needed :')
        ui->OrbitView->TriggerSolarStorm();
    }
}

void mainwindow::OnResetScenario()
{
    operatorActionBox.SetStormWarningVisible(false);
    SaveSharedScenarioState(ScenarioState::Normal);
    stormIsActive = false;

    if (ui->OrbitView != nullptr)
    {
        ui->OrbitView->ResetScenario();
    }
}

void mainwindow::OnRepairSelectedSatellite()
{
    if (selectedSatelliteName.isEmpty())
    {
        // No selected satellite means we do not know who to repair, so we stop here and tell the operator plainly.
        operatorActionBox.ShowMessage("Select a satellite first");
        return;
    }

    // Same repair command path works during a storm, after a storm, or with the backend waking up later.
    if (!InsertRepairCommand(selectedSatelliteName))
    {
        const QString fallbackText = activeDatabasePath.isEmpty() ? "Backend not running" : "Could not send repair command";
        operatorActionBox.ShowMessage(fallbackText);
        return;
    }

    if (ui->OrbitView != nullptr)
    {
        ui->OrbitView->BeginSatelliteRepair(selectedSatelliteName);
    }

    operatorActionBox.ShowMessage("Repair command sent to " + selectedSatelliteName);
}

bool mainwindow::InsertRepairCommand(const QString& satelliteName)
{
    const QString dbpath = !activeDatabasePath.isEmpty() ? activeDatabasePath : ResolveDatabasePath();
    if (dbpath.isEmpty())
    {
        // If we do not know where the DB lives, there is nowhere to drop the command. Bit awkward innit (￣y▽,￣)╭
        return false;
    }

    activeDatabasePath = dbpath;

    sqlite3* db = nullptr;
    sqlite3_stmt* insertStmt = nullptr;
    QByteArray dbPathUtf8 = dbpath.toUtf8();

    if (sqlite3_open_v2(dbPathUtf8.constData(), &db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK)
    {
        // Open failure usually means the backend DB is missing or locked in a bad way. Backend said "not today" I guess.
        sqlite3_close(db);
        return false;
    }

    sqlite3_busy_timeout(db, 1000);

    const char* createSql =
        "CREATE TABLE IF NOT EXISTS ControlCommands("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "satellite_name TEXT NOT NULL, "
        "command TEXT NOT NULL, "
        "created_ms INTEGER NOT NULL, "
        "processed INTEGER NOT NULL DEFAULT 0 );";

    if (sqlite3_exec(db, createSql, nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        // If the command table cannot exist, the repair loop has nowhere to stash orders. Sad times fr fr.
        sqlite3_close(db);
        return false;
    }

    const char* insertSql =
        "INSERT INTO ControlCommands (satellite_name, command, created_ms, processed) "
        "VALUES (?, 'REPAIR', ?, 0)";

    if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK)
    {
        sqlite3_finalize(insertStmt);
        sqlite3_close(db);
        return false;
    }

    QByteArray satNameUtf8 = satelliteName.toUtf8();
    sqlite3_bind_text(insertStmt, 1, satNameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insertStmt, 2, DashboardUiHelper::NowMs());

    bool didInsert = sqlite3_step(insertStmt) == SQLITE_DONE;
    sqlite3_finalize(insertStmt);
    sqlite3_close(db);
    return didInsert;
}

// Fills the top satellite table using the newest telemetry row we have for each satellite. Small but mighty table (❁´◡`❁)
void mainwindow::PopulateSatelliteTable()
{
    ui->SatelliteTable->setRowCount(satellites.size());

    for (int row = 0; row < satellites.size(); ++row)
    {
        const SatelliteRow& sat = satellites[row];

        QTableWidgetItem* nameItem = new QTableWidgetItem(sat.name);
        nameItem->setData(Qt::UserRole, sat.name);
        ui->SatelliteTable->setItem(row, 0, nameItem);

        ui->SatelliteTable->setItem(row, 1, new QTableWidgetItem(sat.linkStatus));
        ui->SatelliteTable->setItem(row, 2, new QTableWidgetItem(QString::number(sat.battery, 'f', 2) + " %"));
        ui->SatelliteTable->setItem(row, 3, new QTableWidgetItem(QString::number(sat.temperature, 'f', 2) + " C"));
    }
}

// Loads the recent packet history for whichever satellite the operator clicked on. This is the "show me receipts" bit.
void mainwindow::PopulateTelemetryTable()
{
    ui->DatabaseTable->setRowCount(0);

    // No selected satellite means no detail history yet, so we quietly bail out. Nothing to yap about yet.
    if (selectedSatelliteName.isEmpty())
    {
        return;
    }

    // Use the same DB path the refresh loop picked, otherwise we might read from the wrong random build folder copy. Been there, very cursed.
    const QString dbpath = !activeDatabasePath.isEmpty() ? activeDatabasePath : ResolveDatabasePath();
    if (dbpath.isEmpty())
    {
        return;
    }

    sqlite3* DB = nullptr;
    sqlite3_stmt* stmt = nullptr;

    QByteArray dbPathUtf8 = dbpath.toUtf8();
    int rc = sqlite3_open_v2(dbPathUtf8.constData(), &DB, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK)
    {
        // If the DB cannot be opened, we do not keep digging.
        // The refresh loop will try again on the next tick, nice and calm (￣y▽,￣)╭
        sqlite3_close(DB);
        return;
    }

    const char* sql =
        "SELECT Sequence, timestamp_ms, received_ms, battery, temperature "
        "FROM Telemetry "
        "WHERE Satellite_name = ? "
        "ORDER BY id DESC "
        "LIMIT 20";

    if (sqlite3_prepare_v2(DB, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        // Grab a short packet trail so the operator can see more than one lonely row.
        QByteArray satNameUtf8 = selectedSatelliteName.toUtf8();
        sqlite3_bind_text(stmt, 1, satNameUtf8.constData(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int row = ui->DatabaseTable->rowCount();
            ui->DatabaseTable->insertRow(row);

            qint64 sequence = sqlite3_column_int64(stmt, 0);
            qint64 timestampMs = sqlite3_column_int64(stmt, 1);
            qint64 receivedMs = sqlite3_column_int64(stmt, 2);
            double battery = sqlite3_column_double(stmt, 3);
            double temperature = sqlite3_column_double(stmt, 4);
            qint64 latencyMs = receivedMs - timestampMs;
            qint64 ageMs = DashboardUiHelper::NowMs() - receivedMs;
            QString linkStatus = DashboardUiHelper::LinkStatus(ageMs);
            QString packetSize = DashboardUiHelper::EstimatePacketSize(selectedSatelliteName, sequence, timestampMs, battery, temperature);
            QString displayTime = DashboardUiHelper::FormatTime(receivedMs);

            ui->DatabaseTable->setItem(row, 0, new QTableWidgetItem(QString::number(sequence)));
            ui->DatabaseTable->setItem(row, 1, new QTableWidgetItem(QString::number(timestampMs)));
            ui->DatabaseTable->setItem(row, 2, new QTableWidgetItem(QString::number(receivedMs)));
            ui->DatabaseTable->setItem(row, 3, new QTableWidgetItem(QString::number(latencyMs)));
            ui->DatabaseTable->setItem(row, 4, new QTableWidgetItem(QString::number(ageMs)));
            ui->DatabaseTable->setItem(row, 5, new QTableWidgetItem(linkStatus));
            ui->DatabaseTable->setItem(row, 6, new QTableWidgetItem(QString::number(battery, 'f', 2) + " %"));
            ui->DatabaseTable->setItem(row, 7, new QTableWidgetItem(QString::number(temperature, 'f', 2) + " C"));
            ui->DatabaseTable->setItem(row, 8, new QTableWidgetItem(packetSize));
            ui->DatabaseTable->setItem(row, 9, new QTableWidgetItem(displayTime));
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(DB);
}

// Pushes the clicked satellite into the labels, telemetry table, and OrbitView.
void mainwindow::ApplySelectedSatellite()
{
    if (selectedSatelliteName.isEmpty())
    {
        ShowNoSelectionState();
        return;
    }

    // Walk the cached list until we find the one the user asked for.
    for (int row = 0; row < satellites.size(); ++row)
    {
        const SatelliteRow& sat = satellites[row];
        if (sat.name != selectedSatelliteName)
        {
            // Skip rows until we hit the currently selected satellite.
            continue;
        }

        QString batteryStatus = DashboardUiHelper::BatteryStatus(sat.battery);
        QString temperatureStatus = DashboardUiHelper::TemperatureStatus(sat.temperature);
        QString healthStatus = DashboardUiHelper::HealthStatus(sat.linkStatus, batteryStatus, temperatureStatus);
        qint64 latency = sat.receivedMs - sat.timestampMs;
        QString packetLossText = "N/A";

        // The storm is still a demo button, but this makes the detail panel feel like it noticed the same event.
        if (stormIsActive)
        {
            // During storm mode we nudge the detail panel language a bit so it matches the scene mood.
            if (sat.name == "SAT_3")
            {
                packetLossText = "Storm Risk";
                healthStatus = "Storm Watch";
            }
            else if (sat.name == "SAT_2")
            {
                packetLossText = "Low Risk";
            }
        }

        ui->SatelliteName_Label->setText(sat.name);
        ui->LinkStatus_Connection->setText(sat.linkStatus);
        ui->PacketLoss_Connection->setText(packetLossText);
        ui->Latency_Data->setText(QString::number(latency) + " ms");
        ui->Battery_Data->setText(QString::number(sat.battery, 'f', 2));
        ui->Temprature_Data->setText(QString::number(sat.temperature, 'f', 2));
        ui->Battery_StatusData->setText(batteryStatus);
        ui->Temp_StatusData->setText(temperatureStatus);
        ui->Health_StatusData->setText(healthStatus);

        ShowTelemetryState();
        PopulateTelemetryTable();

        if (ui->OrbitView != nullptr)
        {
            ui->OrbitView->SetSelectedSatellite(sat.name);
            ui->OrbitView->SetSatelliteLinkStatus(sat.linkStatus);
        }

        return;
    }

    selectedSatelliteName.clear();
    ShowNoSelectionState();
}

// This is the "backend is asleep or empty" state where we hide the juicy telemetry stuff.
void mainwindow::ShowNoTelemetryState(const QString& message)
{
    ui->SatelliteTable->setEnabled(false);
    ui->SatelliteTable->clearSelection();
    ui->SatelliteTable->setRowCount(0);
    ui->DatabaseTable->setEnabled(false);
    ui->DatabaseTable->clearSelection();
    ui->DatabaseTable->setRowCount(0);
    ui->DatabaseTable->setVisible(false);
    ui->layoutWidget->setVisible(false);
    ui->layoutWidget_2->setVisible(false);

    DashboardUiHelper::ClearDetailLabels(ui);

    if (noSatelliteLabel != nullptr)
    {
        noSatelliteLabel->setText(message);
        noSatelliteLabel->show();
        noSatelliteLabel->raise();
    }

    operatorActionBox.SetRepairEnabled(false);
    operatorActionBox.HideMessage();

    if (ui->OrbitView != nullptr)
    {
        // If the backend is gone, the 3D view should also stop pretending a satellite is still selected.
        ui->OrbitView->SetSelectedSatellite(QString());
        ui->OrbitView->SetSatelliteLinkStatus("Disconnected");
    }

    activeDatabasePath.clear();
}

// This is the middle state: telemetry exists, but the user has not picked a satellite yet.
void mainwindow::ShowNoSelectionState()
{
    ui->SatelliteTable->setEnabled(true);
    ui->SatelliteTable->clearSelection();
    ui->DatabaseTable->setEnabled(false);
    ui->DatabaseTable->setRowCount(0);
    ui->DatabaseTable->setVisible(false);
    ui->layoutWidget->setVisible(false);
    ui->layoutWidget_2->setVisible(false);

    DashboardUiHelper::ClearDetailLabels(ui);

    if (noSatelliteLabel != nullptr)
    {
        noSatelliteLabel->setText("Select a satellite to view telemetry");
        noSatelliteLabel->show();
        noSatelliteLabel->raise();
    }

    operatorActionBox.SetRepairEnabled(false);
    operatorActionBox.HideMessage();

    if (ui->OrbitView != nullptr)
    {
        ui->OrbitView->SetSelectedSatellite(QString());
        ui->OrbitView->SetSatelliteLinkStatus("Disconnected");
    }
}

// Makes the detail widgets visible again once a real satellite selection happens.
void mainwindow::ShowTelemetryState()
{
    ui->SatelliteTable->setEnabled(true);
    ui->DatabaseTable->setEnabled(true);
    ui->DatabaseTable->setVisible(true);
    ui->layoutWidget->setVisible(true);
    ui->layoutWidget_2->setVisible(true);

    operatorActionBox.SetRepairEnabled(true);

    if (noSatelliteLabel != nullptr)
    {
        noSatelliteLabel->hide();
    }
}

// Row click handler for the satellite list. Nice and tiny on purpose.
void mainwindow::OnSatelliteRowClicked(int row, int column)
{
    Q_UNUSED(column)

    if (row < 0 || row >= satellites.size())
    {
        return;
    }

    selectedSatelliteName = satellites[row].name;
    ApplySelectedSatellite();
}

// This is the heartbeat of the window. Every tick we re-read the newest per-satellite state from SQLite.
void mainwindow::RefreshTelemetryView()
{
    // Start fresh each pass so the tables always reflect what is actually in the DB right now.
    satellites.clear();
    ui->SatelliteTable->setRowCount(0);
    ui->DatabaseTable->setRowCount(0);

    // Figure out which Soul.db is the live one, because Qt build folders love making clones everywhere.
    activeDatabasePath = ResolveDatabasePath();
    const QString dbpath = activeDatabasePath;
    if (dbpath.isEmpty())
    {
        // No DB path means backend probably is not running yet, so we switch to the friendly empty state.
        selectedSatelliteName.clear();
        ShowNoTelemetryState("Backend not running");
        return;
    }

    sqlite3* DB = nullptr;
    sqlite3_stmt* stmt = nullptr;

    QByteArray dbPathUtf8 = dbpath.toUtf8();
    int rc = sqlite3_open_v2(dbPathUtf8.constData(), &DB, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK)
    {
        // Same idea here: if SQLite will not open, we do not spam logs and panic.
        sqlite3_close(DB);
        selectedSatelliteName.clear();
        ShowNoTelemetryState("Backend not running");
        return;
    }

    const char* sql =
        "SELECT t.Satellite_name, t.sequence, t.battery, t.temperature, t.received_ms, t.timestamp_ms "
        "FROM Telemetry t "
        "WHERE t.id = ("
        "    SELECT MAX(t2.id) "
        "    FROM Telemetry t2 "
        "    WHERE t2.Satellite_name = t.Satellite_name"
        ") "
        "ORDER BY t.id DESC";

    if (sqlite3_prepare_v2(DB, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        qint64 currentTime = DashboardUiHelper::NowMs();

        // Each row here is already "latest row for a satellite", so this becomes the top selection list.
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* satName = sqlite3_column_text(stmt, 0);
            if (satName == nullptr)
            {
                // Null satellite names are junk rows for our UI purposes, so we just step past them.
                continue;
            }

            SatelliteRow sat;
            sat.name = QString::fromUtf8(reinterpret_cast<const char*>(satName));
            sat.sequence = sqlite3_column_int64(stmt, 1);
            sat.battery = sqlite3_column_double(stmt, 2);
            sat.temperature = sqlite3_column_double(stmt, 3);
            sat.receivedMs = sqlite3_column_int64(stmt, 4);
            sat.timestampMs = sqlite3_column_int64(stmt, 5);
            sat.linkStatus = DashboardUiHelper::LinkStatus(currentTime - sat.receivedMs);

            satellites.push_back(sat);
        }
    }
    else
    {
        selectedSatelliteName.clear();
        ShowNoTelemetryState("No telemetry yet");
        sqlite3_finalize(stmt);
        sqlite3_close(DB);
        return;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(DB);

    if (satellites.isEmpty())
    {
        selectedSatelliteName.clear();
        ShowNoTelemetryState("No telemetry yet");
        return;
    }

    PopulateSatelliteTable();

    // Try to preserve the current selection if that satellite still exists in the fresh query result.
    int selectedRow = -1;
    for (int row = 0; row < satellites.size(); ++row)
    {
        if (satellites[row].name == selectedSatelliteName)
        {
            selectedRow = row;
            break;
        }
    }

    if (selectedSatelliteName.isEmpty())
    {
        // Telemetry exists, but no one is picked yet so we stay in the calm state.
        ShowNoSelectionState();
        return;
    }

    if (selectedRow < 0)
    {
        // If the previously selected satellite vanished from the fresh query, we clear selection instead of lying.
        selectedSatelliteName.clear();
        ShowNoSelectionState();
        return;
    }

    {
        QSignalBlocker blocker(ui->SatelliteTable);
        ui->SatelliteTable->selectRow(selectedRow);
    }

    ApplySelectedSatellite();
}

// Hunts down the real Soul.db file instead of blindly trusting one build folder copy.
//Turned out to be a necessity its a hack its useless hack
QString mainwindow::ResolveDatabasePath() const
{
    const QString appDirDb = QDir(QCoreApplication::applicationDirPath()).filePath("Soul.db");
    const QString currentDirDb = QDir::current().filePath("Soul.db");
    const QString repoRootDb = "C:/SOUL/Soul.db";
    const QString buildRootDb = "C:/SOUL/build/Soul.db";

    const QStringList candidates = {
        activeDatabasePath,
        appDirDb,
        currentDirDb,
        repoRootDb,
        buildRootDb,
        "C:/SOUL/build/Debug/Soul.db",
        "C:/SOUL/build/Desktop_Qt_6_11_0_llvm_mingw_64_bit-Debug/Soul.db",
        "C:/SOUL/cmake-build-debug/Soul.db"
    };

    QString bestPathWithRows;
    QString bestEmptyPath;
    QDateTime bestWithRowsTime;
    QDateTime bestEmptyTime;

    // We check a few likely places, then prefer the newest DB that actually has telemetry rows in it.
    for (const QString& path : candidates)
    {
        QFileInfo info(path);
        if (!info.exists() || info.size() == 0)
        {
            // Missing or zero-byte DB files are just decoys, so we skip them early.
            continue;
        }

        sqlite3* DB = nullptr;
        sqlite3_stmt* stmt = nullptr;
        QByteArray dbPathUtf8 = path.toUtf8();

        if (sqlite3_open_v2(dbPathUtf8.constData(), &DB, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
        {
            // Some candidates are stale or locked weirdly, and that is okay, we just keep hunting.
            sqlite3_close(DB);
            continue;
        }

        bool hasTelemetryTable = false;
        int telemetryRowCount = 0;

        const char* hasTableSql =
            "SELECT 1 "
            "FROM sqlite_master "
            "WHERE type = 'table' AND name = 'Telemetry' "
            "LIMIT 1";

        if (sqlite3_prepare_v2(DB, hasTableSql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            hasTelemetryTable = (sqlite3_step(stmt) == SQLITE_ROW);
        }

        sqlite3_finalize(stmt);
        stmt = nullptr;

        if (hasTelemetryTable)
        {
            // We want the DB that is actually alive, not the empty decoy one hiding in another build folder.
            const char* countSql = "SELECT COUNT(*) FROM Telemetry";
            if (sqlite3_prepare_v2(DB, countSql, -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
            {
                telemetryRowCount = sqlite3_column_int(stmt, 0);
            }
        }

        sqlite3_finalize(stmt);
        sqlite3_close(DB);

        if (!hasTelemetryTable)
        {
            // If the DB exists but has no Telemetry table, it is not our active backend file.
            continue;
        }

        if (telemetryRowCount > 0)
        {
            // Prefer the newest DB that actually has telemetry rows.
            if (bestPathWithRows.isEmpty() || info.lastModified() > bestWithRowsTime)
            {
                bestPathWithRows = path;
                bestWithRowsTime = info.lastModified();
            }
        }
        else if (bestEmptyPath.isEmpty() || info.lastModified() > bestEmptyTime)
        {
            // Keep one empty-but-valid fallback path around in case the backend created the DB before writing rows.
            bestEmptyPath = path;
            bestEmptyTime = info.lastModified();
        }
    }

    if (!bestPathWithRows.isEmpty())
    {
        return bestPathWithRows;
    }

    return bestEmptyPath;
}

// Placeholder for later if database setup gets moved out of the window class.
const char * mainwindow::LoadDatabase()
{
    //FIXME  - Separate database logic later here //

    return nullptr;
}
