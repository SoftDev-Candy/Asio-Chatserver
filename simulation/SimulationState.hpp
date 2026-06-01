//
// Created by Candy on 4/9/26.
//

#ifndef SOUL_SIMULATIONSTATE_HPP
#define SOUL_SIMULATIONSTATE_HPP
#include"../Common/TelemetryFrame.hpp"
#include "../Common/ScenarioState.hpp"
#include <optional>

// This is the simple mood board for a satellite.
// Each mode nudges the telemetry in a different direction.
enum class SatelliteMode
{
    Nominal,
    PowerDrain,
    Overheating,
    SignalLoss
};

//FIXME - Everything is public for now if time we hide more
class SimulationState
{
private:
    // These mirror the telemetry frame fields we care about keeping alive between sends.
    // sat_id tells us who we are pretending to be.
    // sequence keeps climbing so the backend sees a normal stream.
    // battery and temp_c are the actual changing health values.
    std::string sat_id{};
    uint64_t sequence{};
    float battery{};
    float temp_c{};
    // These two are the behavior sliders for the simple simulation.
    // drain_per_frame says how much battery disappears every tick.
    // drift_per_frame says how much the temperature moves every tick.
    float battery_drain_per_frame{};
    float temperature_drift_per_frame{};
    // mode is the headline behavior for this satellite.
    // tick_counter helps us fake periodic events like signal loss.
    // signal_loss_ticks_left is the little countdown for "skip the next few sends".
    SatelliteMode mode = SatelliteMode::Nominal;
    int tick_counter = 0;
    int signal_loss_ticks_left = 0;
    int repair_ticks_left = 0;

public:
    // Builds one small satellite state machine with its own starting health values and drift behavior.
    SimulationState(
        const std::string& id,
        float bat,
        float temp,
        SatelliteMode satMode = SatelliteMode::Nominal,
        float batteryDrainPerFrame = 1.0f,
        float temperatureDriftPerFrame = -0.02f);

    // Generates the next telemetry frame after applying the simple behavior rules.
    // If we are faking signal loss right now, this can return no frame for this tick.
    std::optional<TelemetryFrame> MakeNextFrame(ScenarioState scenarioState);

    // Lets the sender match pending repair commands to the right satellite.
    const std::string& GetSatelliteId() const;

    // Gives the satellite a small recovery boost so the repair button actually does something.
    void ApplyRepairCommand();

};

#endif //SOUL_SIMULATIONSTATE_HPP
