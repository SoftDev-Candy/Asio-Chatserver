//
// Created by Candy on 2/16/26.
//

#ifndef CHATSERVER_CHATSERVER_H
#define CHATSERVER_CHATSERVER_H

#include <array>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include "../Common/FrameCodec.hpp"
#include "../Common/TelemetryFrame.hpp"

namespace{
    using boost::asio::ip::tcp;
    using std::mutex;
}

class SatelliteSim
{
public:
    // Builds the server with the address/port it should listen on.
    SatelliteSim( std::string add , unsigned short int port_i );

    // Main backend loop that keeps accepting clients and spinning handlers forever.
    void RunServer();

    // Handles one client socket: decode frame, validate JSON, write DB, send ACK.
    void ReceiveTelemetry(tcp::socket &socket) const;

    // Shared framing helper so sender and receiver speak the same wire language.
    FrameCodec Frame;

private:
    // Small setup helper so the constructor does not become one giant wall of network setup text.
    tcp::acceptor create_tcp_acceptor();

    // Basically the I/O engine for Boost.Asio.
    boost::asio::io_context io_context;

    // The acceptor is the thing that actually listens for incoming TCP clients.
    tcp::acceptor acceptor;

    // Keeps count of clients that have connected so far.
    uint_fast64_t clientCount = 0 ;
    std::string address;
    unsigned short int PORT;

    // Stores the newest frame for each satellite in memory.
    static std::unordered_map<std::string ,TelemetryFrame>TelemetryStateMap ;
};

#endif //CHATSERVER_CHATSERVER_H
