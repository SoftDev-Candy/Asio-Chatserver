//
// Created by Candy on 4/5/26.
//

#ifndef SOUL_DATABASE_HPP
#define SOUL_DATABASE_HPP

#include <cstdint>
#include "../Common/TelemetryFrame.hpp"
#include "../external/sqlite/sqlite3.h"

class Database
{
private:

public:
    // Opens the SQLite file and makes sure the needed tables exist before the server starts writing rows.
    static int Database_init();
    static int Create_DB();
    static int CreateTable();
    static void Terminate();//Closes database and will terminate any processes that need to be terminated

    // Writes one telemetry frame plus the receiver-side timestamp into the Telemetry table.
    static const int InsertTelemetry(const TelemetryFrame& tframe , uint64_t received_ms) ;

    //FIXME -- I need to do something about keeping this static not a good solution making it static or global
    static sqlite3* DB; // Database connection obj
};

#endif //SOUL_DATABASE_HPP
