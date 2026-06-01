//
// Created by Candy on 4/5/26.
//

#include "Database.hpp"

#include <iostream>
//Static Declaration
sqlite3* Database::DB;

int Database::Database_init()
{
    // One-time DB setup path for the receiver.
    // Open first, create tables second, then the rest of the backend can write without thinking about setup.
    Create_DB();
    CreateTable();
    return 0;
}

//FIXME -- ADD CREATE DATABASE LOGIC HERE
int Database::Create_DB()
{
    int rc = sqlite3_open("Soul.db",&DB);//Call the sqlite constructor

    if (rc != SQLITE_OK)
    {
        // If the DB cannot open, the receiver has nowhere to persist telemetry, so we stop early.
        std::cerr<<"Failed to open database ERROR  : "<<sqlite3_errmsg(DB)<<"\n";
        sqlite3_close(DB);
        return 1;
    }
return 0;
}

//FIXME -- ADD CREATE TABLE LOGIC HERE
int Database::CreateTable()
{    //Create SQL table named telemetry if it doesn't exist already//

    //FIXME -- AFTER DATABASE IS CREATED IT NEEDS TO CHECK IF THE DATA WAS CHANGED OR NOT it doesnt add another coloumn on its own

    const char* sql =   "CREATE TABLE IF NOT EXISTS Telemetry("
                        "id                 INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "Satellite_name     TEXT NOT NULL, "
                        "Sequence           INTEGER NOT NULL, "
                        "timestamp_ms       INTEGER NOT NULL, "
                        "battery            REAL NOT NULL, "
                        "temperature        REAL NOT NULL, "
                        "received_ms        INT NOT NULL );"
                        "CREATE TABLE IF NOT EXISTS ControlCommands("
                        "id                 INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "satellite_name     TEXT NOT NULL, "
                        "command            TEXT NOT NULL, "
                        "created_ms         INTEGER NOT NULL, "
                        "processed          INTEGER NOT NULL DEFAULT 0 );";

    //Apparently this exec function cant take strings so we use only chars now
    //FIXME -- WRAP THIS WHOLE SEGMENT IN A TRY AND CATCH MAYBE //
    char* error_msg =  nullptr;
    //call exec on the database and the SQL
    int rc = sqlite3_exec(DB,sql,nullptr,nullptr,&error_msg);
    if (rc != SQLITE_OK)
    {
        // Table creation failing usually means the DB file exists but is in a weird/bad state.
        std::cerr<<"Failed to Create Table in database ERROR  : "<<error_msg<<"\n";
        sqlite3_close(DB);
        return 1;
    }
    else
    {
        std::cout<<"Database opened/Created and is ready to be accessed"<<std::endl;
    }

    return 0;
}

void Database::Terminate()
{

    sqlite3_close(DB);
    std::cout<<"Database is closed as end of while loop -- Quit condition is applied"<<std::endl;

}

const int Database::InsertTelemetry(const TelemetryFrame &tframe , uint64_t received_ms)
{

    //Prepare statement obj
    sqlite3_stmt* stmt = nullptr;

    //To insert one row into telemetry and put 5 values in the coloumn
    const char* sql =
        "INSERT INTO Telemetry (Satellite_name,Sequence,timestamp_ms,battery,temperature,received_ms)"
        "VALUES(?,?,?,?,?,?);";

    //Builds the statement object from the sql text from above
    int rc = sqlite3_prepare_v2(DB,sql,-1,&stmt,nullptr);
    if(rc != SQLITE_OK)
    {
        // Prepare failure means the insert statement itself did not compile in SQLite land.
        return 1;
    }


    //Fill in the placeholder values we set up in the SQL as '?' using bind
    sqlite3_bind_text(stmt,1,tframe.sat_id.c_str(),-1,SQLITE_TRANSIENT);//Converting to c.str as we need cstyle string not C++
    sqlite3_bind_int64(stmt,2,tframe.sequence);
    sqlite3_bind_int64(stmt,3,tframe.timestamp_ms);
    sqlite3_bind_double(stmt,4,tframe.battery);
    sqlite3_bind_double(stmt,5,tframe.temp_c);
    sqlite3_bind_int64(stmt,6,received_ms);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(rc != SQLITE_DONE)
    {
        // SQLITE_DONE is the happy path here.
        // Anything else means the row never actually landed in the DB.
        std::cerr<<"Ran into an Error during insertion of frame into the database"<<std::endl;
        return 1;

    }

std::cout<<"Data has been added to the Database"<<std::endl;

return 0;
}



