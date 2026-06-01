//
// Created by Candy on 4/9/26.
//

#include <chrono>
#include <algorithm>
#include "SimulationState.hpp"

//Constructor for Simulation State Class
SimulationState::SimulationState(
    const std::string &id,
    float bat,
    float temp,
    SatelliteMode satMode,
    float batteryDrainPerFrame,
    float temperatureDriftPerFrame)
{
    // This constructor is satellite personality loader each one with some other disorder in some terms you could them special
    sat_id = id;
    battery = bat;
    temp_c = temp;
    sequence = 0;
    mode = satMode;
    battery_drain_per_frame = batteryDrainPerFrame;
    temperature_drift_per_frame = temperatureDriftPerFrame;
}

//Controls how to simulation works --Simple but effective for my use cases
std::optional<TelemetryFrame> SimulationState::MakeNextFrame(ScenarioState scenarioState)
{
    tick_counter++;

    // New loop, new packet, so bump the sequence first.
    sequence++;

    // These are the actual values we will use for this specific tick after the mode tweaks them.
    float batteryDropThisTick = battery_drain_per_frame;
    float tempMoveThisTick = temperature_drift_per_frame;
    bool stormIsActive = (scenarioState == ScenarioState::SolarStormActive);
    bool recoveryIsActive = (scenarioState == ScenarioState::Recovery);
    bool repairIsWorking = repair_ticks_left > 0;

    // This is where the active scenario changes how the next packet should look.
    // Normal = mostly chill.
    // Storm = SAT_2 and SAT_3 start taking extra damage.
    // Recovery = stop the extra storm effects and let temperature calm down a bit.
    if (mode == SatelliteMode::PowerDrain && stormIsActive)
    {
        batteryDropThisTick += 0.80f;
    }
    else if (mode == SatelliteMode::Overheating && stormIsActive)
    {
        tempMoveThisTick += 0.10f;
    }
    else if (mode == SatelliteMode::SignalLoss)
    {
        if (stormIsActive)
        {
            // SAT_3 heats up more during the storm and can briefly stop sending.
            tempMoveThisTick += 0.16f;

            // Every so often, pretend the radio goes quiet for a couple of frames.
            // Repair mode blocks this little radio tantrum for a few sends.
            if (!repairIsWorking && signal_loss_ticks_left <= 0 && tick_counter % 7 == 0)
            {
                signal_loss_ticks_left = 2;
            }
        }
        else if (recoveryIsActive)
        {
            // Recovery does not magically heal everything, but it does stop the storm from cooking SAT_3 harder.
            tempMoveThisTick -= 0.05f;
            signal_loss_ticks_left = 0;
        }
        else
        {
            // No storm, no radio nonsense.
            signal_loss_ticks_left = 0;
        }
    }

    // Repair is a small temporary buff, not magic.
    // It cuts down the bad stuff for a few ticks so the operator sees a recovery trend.
    if (repairIsWorking)
    {
        batteryDropThisTick *= 0.35f;
        tempMoveThisTick -= 0.12f;
        signal_loss_ticks_left = 0;
        repair_ticks_left--;
    }

    // this here is a an engine it goes chooooo chooo: each satellite burns battery and shifts temperature at its own pace.
    battery = std::max(0.0f, battery - batteryDropThisTick);
    temp_c += tempMoveThisTick;

    // Grab "now" in milliseconds so the receiver can still compute age and latency like before.
    //FIXME - Technically need to have something like a public function I called this particular logic 3 times now lol
    auto duration  = std::chrono::system_clock::now().time_since_epoch();
    //Conversion to store it in time stamp
    uint64_t timestamp_val = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    if (mode == SatelliteMode::SignalLoss && signal_loss_ticks_left > 0)
    {
        signal_loss_ticks_left--;
        return std::nullopt;
    }

    // Stuff all the updated values into the outgoing telemetry frame.---yes tf ... THE FLIPPER or WHAT DAAAA FAAAAA
    TelemetryFrame tf;
    tf.sat_id = sat_id;
    tf.sequence = sequence;
    tf.battery = battery;
    tf.temp_c = temp_c;
    tf.timestamp_ms = timestamp_val;

    return tf;
}

const std::string& SimulationState::GetSatelliteId() const
{
    return sat_id;
}

void SimulationState::ApplyRepairCommand()
{
    // Repair is intentionally simple:
    // bump the battery a little, cool the satellite down, and stop the current signal-drop streak.
    battery = std::min(100.0f, battery + 8.0f);
    temp_c = std::max(18.0f, temp_c - 5.5f);
    signal_loss_ticks_left = 0;
    repair_ticks_left = 6;
}
