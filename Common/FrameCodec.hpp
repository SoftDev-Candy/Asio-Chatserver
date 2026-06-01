//
// Created by Candy on 3/6/26.
//

#ifndef SOUL_FRAMECODEC_HPP
#define SOUL_FRAMECODEC_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <boost/asio/ip/tcp.hpp>

class FrameCodec
{
    private:
    //Nothing to add here yet//

    public:
    // Adds a 4-byte length header in front of the JSON payload.
    // This is what lets the socket side know exactly how many bytes belong to one frame.
    static std::vector<uint8_t> EncodeFrame(std::string Json_str);

    // Reads one whole frame from the socket: header first, payload second.
    // If anything looks invalid, we return an empty string and let the caller handle it.
    static std::string DecodeFrame(boost::asio::ip::tcp::socket& socket);

};


#endif //SOUL_FRAMECODEC_HPP
