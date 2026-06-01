//
// Created by Candy on 2/16/26.
//

#include "TelemetryHub.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "../ReceiverServer/SatelliteSim.hpp"
#include "../Common/ScenarioState.hpp"
#include "../simulation/SimulationState.hpp"
#include "../external/sqlite/sqlite3.h"
using boost::asio::ip::tcp;

namespace
{
// The sender and the UI both need to find the same database file or the repair button looks fake.
std::string ResolveControlDbPath()
{
    namespace fs = std::filesystem;

    const std::vector<std::string> candidates = {
        "C:/SOUL/Soul.db",
        (fs::current_path() / "Soul.db").string(),
        "C:/SOUL/build/Soul.db",
        "C:/SOUL/build/Debug/Soul.db",
        "C:/SOUL/cmake-build-debug/Soul.db"
    };

    fs::file_time_type newestTime{};
    std::string bestPath;

    for (const std::string& path : candidates)
    {
        std::error_code timeError;
        if (!fs::exists(path))
        {
            continue;
        }

        fs::file_time_type changedAt = fs::last_write_time(path, timeError);
        if (!bestPath.empty() && timeError)
        {
            continue;
        }

        if (bestPath.empty() || (!timeError && changedAt > newestTime))
        {
            bestPath = path;
            newestTime = changedAt;
        }
    }

    return bestPath;
}

// This is the tiny mailbox loop for operator commands.
// UI drops REPAIR rows into SQLite, sender picks them up, applies them, then marks them done.
void ApplyPendingRepairCommands(std::array<SimulationState, 3>& satellites)
{
    const std::string dbPath = ResolveControlDbPath();
    if (dbPath.empty())
    {
        return;
    }

    sqlite3* db = nullptr;
    sqlite3_stmt* readStmt = nullptr;
    sqlite3_stmt* markStmt = nullptr;

    if (sqlite3_open_v2(dbPath.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return;
    }

    sqlite3_busy_timeout(db, 1000);

    const char* createSql =
        "CREATE TABLE IF NOT EXISTS ControlCommands("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "satellite_name TEXT NOT NULL, "
        "command TEXT NOT NULL, "
        "created_ms INTEGER NOT NULL, "
        "processed INTEGER NOT NULL DEFAULT 0 );";

    sqlite3_exec(db, createSql, nullptr, nullptr, nullptr);

    const char* readSql =
        "SELECT id, satellite_name "
        "FROM ControlCommands "
        "WHERE processed = 0 AND command = 'REPAIR' "
        "ORDER BY id ASC";

    if (sqlite3_prepare_v2(db, readSql, -1, &readStmt, nullptr) != SQLITE_OK)
    {
        sqlite3_finalize(readStmt);
        sqlite3_close(db);
        return;
    }

    std::vector<int> handledIds;

    while (sqlite3_step(readStmt) == SQLITE_ROW)
    {
        int commandId = sqlite3_column_int(readStmt, 0);
        const unsigned char* satName = sqlite3_column_text(readStmt, 1);
        if (satName == nullptr)
        {
            continue;
        }

        const std::string satelliteName = reinterpret_cast<const char*>(satName);

        for (SimulationState& satellite : satellites)
        {
            if (satellite.GetSatelliteId() != satelliteName)
            {
                continue;
            }

            satellite.ApplyRepairCommand();
            handledIds.push_back(commandId);
            break;
        }
    }

    sqlite3_finalize(readStmt);
    readStmt = nullptr;

    const char* markSql = "UPDATE ControlCommands SET processed = 1 WHERE id = ?";
    if (sqlite3_prepare_v2(db, markSql, -1, &markStmt, nullptr) == SQLITE_OK)
    {
        for (int commandId : handledIds)
        {
            sqlite3_reset(markStmt);
            sqlite3_clear_bindings(markStmt);
            sqlite3_bind_int(markStmt, 1, commandId);
            sqlite3_step(markStmt);
        }
    }

    sqlite3_finalize(markStmt);
    sqlite3_close(db);
}
}


void TelemetryHub::SendTelemetry(boost::asio::ip::tcp::socket &socket)
{
    // This little codec dude keeps the framed TCP protocol exactly the same as before.
    FrameCodec Frame;

    // Three satellites, same radio pipe, slightly different personalities.
    // SAT_1 is the chill one.
    // SAT_2 is the one that suffers more when the storm kicks off.
    // SAT_3 is the flaky one that heats up and can lose signal during the storm.
    std::array<SimulationState, 3> satellites = {
        SimulationState("SAT_1", 100.0f, 44.6f, SatelliteMode::Nominal, 0.10f, -0.01f),
        SimulationState("SAT_2", 100.0f, 42.5f, SatelliteMode::PowerDrain, 0.12f, 0.00f),
        SimulationState("SAT_3", 100.0f, 39.0f, SatelliteMode::SignalLoss, 0.12f, 0.01f)
    };

    while (true)
    {
        // Read the shared scenario once per send round so the UI button can nudge the whole simulation.
        ScenarioState scenarioState = LoadSharedScenarioState();
        ApplyPendingRepairCommands(satellites);

        // One loop now means "every satellite gets a turn" instead of just SAT_1 hogging the spotlight.
        for (SimulationState& satellite : satellites)
        {
            // Ask the simulation for the next fake-but-useful telemetry frame.
            std::optional<TelemetryFrame> nextFrame = satellite.MakeNextFrame(scenarioState);

            // No frame here means "pretend the signal dropped", not "the whole client died".
            if (!nextFrame.has_value())
            {
                continue;
            }

            TelemetryFrame tf = *nextFrame;

            /*
            //Lets get the current point in time from clock//
            //Lets get the duration Epoch from its start
            auto duration  = std::chrono::system_clock::now().time_since_epoch();

            //Conversion to store it in time stamp
            uint64_t timestamp_val = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            */

            // Turn the frame into JSON first because that is what the framing layer already expects.
            std::string TelemetryJSON = tf.ToJson();

            // Wrap the payload in the framed protocol and ship it out.
            auto encoded = Frame.EncodeFrame(TelemetryJSON);
            boost::asio::write(socket, boost::asio::buffer(encoded));

            // Same old handshake, just repeated for whichever frames actually got sent this round.
            auto decoded = Frame.DecodeFrame(socket);
            if (decoded == "")
            {
                std::cerr<<"Decoded Ack from the server is invalid";
            }
            else
            {
                std::cout<<"Ack Type "<<decoded<<std::endl;
            }
        }

        // Tiny pause so the sender does not become an absolute menace.
        //Simple Threading Underneath ...I should get into knitting ..//
        //I really don't think this is a good practice do want to see how pro's do this tho ...lets do steady clock
        //and sleep until to slow the entire thread down so we dont get spammed constantly//
        //First define the clock type to use
        using Clock = std::chrono::steady_clock;

        //Second set the current time
        Clock::time_point rightNow  = Clock::now();

        //Third Calculate the time to wake_up the thread  //
        Clock::time_point wakeupTime = rightNow + std::chrono::seconds(2);

        //Call std::this_thread::sleep_until() with the future time point
        std::this_thread::sleep_until(wakeupTime);


    }

}

 int TelemetryHub::runClient()
{
    try
    {
        // The client side is still intentionally boring: make socket, connect once, start sending forever.
        boost::asio::io_context io_context;
        unsigned short int PORT = 5000;

        tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"),PORT);

        //Create a socket
        tcp::socket socket(io_context);

        socket.connect(endpoint);

        SendTelemetry(socket);

        return 0; // success

    }catch(std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
        return 1; //Error
    }

}


