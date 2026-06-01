//
// Created by Candy on 2/27/26.
//

#ifndef SOUL_TELEMETRYFRAME_H
#define SOUL_TELEMETRYFRAME_H

#include <cstdint>
#include <optional>
#include <string>
#include <boost/json.hpp>

// This is the packet that travels from sender -> receiver -> DB -> UI.
// If this struct changes, almost every layer in the project needs to agree with it.
struct TelemetryFrame
{
    std::string sat_id{};//Satellite id to show distinction between Satellite.

    //sequence helps detect application-level message loss that is lost frames or reordered frames or duplicates etc.
    uint64_t sequence{};

    //Message Generation time at the satellite you can use it to calculate delay , staleness , frame age etc.
    uint64_t timestamp_ms{};

    //System health basically to see if the system is dying or overheating or if something abnormal
    float battery{};

    //Same thing as the battery can be used to measure a lot of things
    float temp_c{};

    // Packs the struct into JSON because that is the format the TCP framing layer ships around.
     std::string ToJson()const ;

    // Unpacks JSON back into a telemetry frame.
    // Optional is used so bad payloads can fail safely instead of blowing up the whole receive loop.
    static std::optional<TelemetryFrame> FromJson(const std::string& json);
};

#endif //SOUL_TELEMETRYFRAME_H
