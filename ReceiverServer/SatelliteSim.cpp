//
// Created by Candy on 2/16/26.
//

#include<thread>
#include<chrono>
#include "SatelliteSim.hpp"
#include "../Common/TelemetryFrame.hpp"
#include "../Database/Database.hpp"

using boost::asio::ip::tcp;
namespace{
    std::mutex coutMutex;
}

std::unordered_map<std::string ,TelemetryFrame>SatelliteSim::TelemetryStateMap;

SatelliteSim::SatelliteSim(std::string add, unsigned short int port_i)
    :io_context(),
     acceptor(io_context),
     address(std::move(add)),
     PORT(port_i)
{
    acceptor = create_tcp_acceptor();
}

void SatelliteSim::RunServer()
{
    Database::Database_init();
    // This loop is the server's day job:
    // wait for a client, hand it off to a thread, then go right back to waiting again.
    while (true)
    {
        //Create a socket here//
        tcp::socket socket(io_context);

        if (clientCount == 0)
        {
            std::cout<<" Server is waiting to connect "<<std::endl;
        }
        else
        {
            // Once the first client showed up, the log becomes a little more explicit about repeats.
            std::cout<<" Server: waiting for a new client...ClientNum :  "<<clientCount<<std::endl;
        }

        //Make the acceptor wait till Client connects//
        acceptor.accept(socket);


        std::thread ClientThread(
            [this](tcp::socket s)
            {
                //call the client server
                ReceiveTelemetry(s);
            },
            std::move(socket)
    );
ClientThread.detach();
        clientCount++;

        std::cout<<"Client Connected Successfully ! ... "<<std::endl;
    }

}

tcp::acceptor SatelliteSim::create_tcp_acceptor()
{
    tcp::endpoint endpoint(boost::asio::ip::make_address(address),PORT);

    //Create an acceptor to start connection request if sent //
    tcp::acceptor acceptor(io_context,endpoint);

    //Adding an Optional check below //
    tcp::endpoint local = acceptor.local_endpoint();
    std::cout<<"Server is listening on: "<<local.address().to_string()<< " : "<<local.port()<<std::endl;

    return acceptor;

}

void SatelliteSim::ReceiveTelemetry(tcp::socket &socket) const
{

    //Make an optional check here to see if the which client has connected from their address and port//
    tcp::endpoint remote = socket.remote_endpoint();
    std::cout<<"The cliented is connected from "<<remote.address().to_string()<<" : "<<remote.port()<<std::endl;

    //While loop so server stays on and keeps receiving data from multiple clients
    while (true)
    {
        //Decode the received bytes from the socket into a string//
        std::string Decoded = Frame.DecodeFrame(socket);

        if (Decoded == "")
        {
            // Empty decode means the peer closed up shop or the frame broke badly enough to stop the loop.
            std::cerr<<"The Decoded string was empty"<<std::endl;
            break;
        }

        //Store it in a variable and check if its valid, if it is valid print the data received from frame
        auto frame = TelemetryFrame::FromJson(Decoded);
        if (frame)
        {
            const TelemetryFrame& convertedFrame = *frame;
            //Push the frame in unordered-map by Sat_id //
            TelemetryStateMap[convertedFrame.sat_id] = convertedFrame;

            //Calculate current time here //
            auto duration  = std::chrono::system_clock::now().time_since_epoch();
            //Conversion to store it in time stamp
            uint64_t received_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            //Call insert telemetry with the current frame and received message time stamp
            Database::InsertTelemetry(convertedFrame , received_ms);
        }
        if (!frame)
        {
            // Bad JSON should not kill the whole client connection instantly.
            // We skip this one and keep listening for the next clean frame (￣y▽,￣)╭
            std::cerr<<"Frame was invalid";
            continue;

        }else
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout<<"HandleClient in thread id"<<std::this_thread::get_id()<<std::endl;

            std::cout<<frame->sat_id<<"\n";
            std::cout<<frame->sequence<<"\n";
            std::cout<<frame->timestamp_ms<<"\n";
            std::cout<<frame->battery<<"\n";
            std::cout<<frame->temp_c<<"\n";
        }

        //Creating a JSON namespace to send Ack to the client of telemetry
        boost::json::object object;
        //Building Json Objects to send Ack
        object["type"] = "ack";
        object["sequence"] = frame->sequence;

        //Serializing and encoded the object
        std::string serializedJson = boost::json::serialize(object);
        auto encoded = Frame.EncodeFrame(serializedJson);

        //Writing the object in the socket and preparing it to send to the client
        boost::asio::write(socket, boost::asio::buffer(encoded));

    }//While loop bracket goes here THE END

    //FIXME -- This is not getting reached because well there is no quit condition. Do I even need it is the question hmm
Database::Terminate();

}
