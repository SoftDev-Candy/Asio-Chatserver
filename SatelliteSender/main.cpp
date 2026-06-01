//
// Created by Candy on 2/7/26.
//

#include<iostream>
#include<array>
#include "TelemetryHub.hpp"


int main()
{
    // Client bootstrap is super small on purpose.
    // It just starts the sender loop and lets TelemetryHub do the heavy lifting.
    TelemetryHub::runClient();

}
