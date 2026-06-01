//
// Created by Candy on 2/7/26.
//

#include"SatelliteSim.hpp"
int main()
{
    try {

        // Tiny server bootstrap.
        // Pick a port, build the backend, then let it run forever until somebody closes the app.
        unsigned short int PORT = 5000;
        SatelliteSim Server("127.0.0.1" , PORT);
        Server.RunServer();

        return 0;

    }catch(std::exception& e) {

        std::cout<<e.what()<<std::endl;

    return 1;

    }

}
