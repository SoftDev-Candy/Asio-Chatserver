//
// Created by Candy on 3/2/26.
//

#include "TelemetryFrame.hpp"
#include <iostream>

namespace json = boost::json;

//Serialization of  the bytes
std::string TelemetryFrame::ToJson() const
{

  //Create a JSON Object here//
  json::object obj;

  //Setting the JSON obj value to be serialized
  obj["sat_id"]     =  this->sat_id;
  obj["sequence"]     =  this->sequence;
  obj["timestamp_ms"] =  this->timestamp_ms;
  obj["battery"]      =  this->battery;
  obj["temp_c"]       =  this->temp_c;

  std::string serialized_str = json::serialize(obj);
  return serialized_str;
}

//Switched the return type to optional to figure out if we have bad frames in place
//Deserialization
std::optional<TelemetryFrame> TelemetryFrame::FromJson(const std::string &json_string)
{

  json::error_code ec;

  //Parsing the string and storing its value
  json::value val_from_string = json::parse(json_string, ec);

  if (ec)
  {

    std::cerr<<"Failed to parse Json "<<ec.message()<<std::endl;
    // Parse failure here usually means the socket data was junk or half-broken.
    // We bail out quietly with nullopt so the caller can decide what to do next.
    return std::nullopt;

  }

  if (!val_from_string.is_object())
  {
        // If it is not even a JSON object, none of the field lookups below are worth attempting.
        std::cerr<<"Invalid JSON :expected object \n";
        return std::nullopt;
  }

  //  Created an obj from Value
  json::object& val_obj = val_from_string.as_object(); // as_object instead of get object

 //To check and validate if obj contain their respective values
  if (!val_obj.contains("sat_id"))
  {
    // We validate field-by-field so bad payloads are easy to diagnose while testing.
    std::cerr << "Missing field: sat_id\n";
    return std::nullopt;
  }

  if (!val_obj.contains("sequence"))
  {
    std::cerr << "Missing field: sequence\n";
    return std::nullopt;
  }

  if (!val_obj.contains("timestamp_ms"))
  {
    std::cerr << "Missing field: timestamp_ms\n";
    return std::nullopt;
  }

  if (!val_obj.contains("battery"))
  {
    std::cerr << "Missing field: battery\n";
    return std::nullopt;
  }

  if (!val_obj.contains("temp_c"))
  {
    std::cerr << "Missing field: temp_c\n";
    return std::nullopt;
  }

  try
  {
    //Telemetry Frame struct object //
    TelemetryFrame tf;

    //Setting values in telemetry frame's object using value_to function for safe deserialization
  tf.sat_id =  json::value_to<std::string>(val_obj.at("sat_id"));
  tf.sequence =  json::value_to<uint64_t>(val_obj.at("sequence"));
  tf.timestamp_ms =  json::value_to<uint64_t>(val_obj.at("timestamp_ms"));
  tf.battery =  json::value_to<float>(val_obj.at("battery"));
  tf.temp_c =  json::value_to<float>(val_obj.at("temp_c"));

    //Don't forget to return it
    return tf;
  }

  catch (const std::exception& e)
  {
    std::cerr<<"Error Accessing JSON field's"<<e.what()<<std::endl;
  }

return std::nullopt;
}
