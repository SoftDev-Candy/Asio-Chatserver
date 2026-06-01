    //
    // Created by Candy on 3/18/26.
    //

    //FIXME -- File here only for me to test the code nothing else nothing more //

    #include <iostream>
    #include "../Common/FrameCodec.hpp"
    #include "../Common/TelemetryFrame.hpp"

    int main()
    {
        // Tiny scratch test file for the two most basic shared pieces:
        // telemetry JSON conversion and the frame-size header codec.
        TelemetryFrame tf;
        tf.sat_id = "SAT_1";
        tf.sequence = 482094824;
        tf.timestamp_ms = 749284738947;
        tf.battery = 89.3f;
        tf.temp_c = 44.6f;

        std::string tojsontest1= tf.ToJson();
        std::printf("%s\n", tojsontest1.c_str());

        auto encode = tf.FromJson(tojsontest1);
        if (encode)
        {
            // If this prints the same values we started with, the JSON round-trip behaved itself.

            std::cout<<encode->sat_id<<"\n";
            std::cout<<encode->sequence<<"\n";
            std::cout<<encode->timestamp_ms<<"\n";
            std::cout<<encode->battery<<"\n";
            std::cout<<encode->temp_c<<"\n";

        }

        FrameCodec frame;
        auto encoded = frame.EncodeFrame(tojsontest1);
        auto json_size = static_cast<uint32_t>(tojsontest1.size());
        std::cout<<"Size of JSON is :"<<json_size<<"\n";
        std::cout<<"Size of Encode_JSON is :"<<encoded.size()<<"\n";

        if (encoded.size() == 4 + json_size)
        {
            // Header is 4 bytes, payload is the raw JSON string.
            // So this check is the quick "math still mathing" sanity test.
            for (size_t i = 0 ; i < 4 ; i++)
            {
                std::cout<<static_cast<int>(encoded[i])<<" ";
            }
        }

        return 0;
    }
