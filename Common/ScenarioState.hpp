//
// Shared scenario state for the UI and sender.
// Tiny file bridge on purpose so the demo can sync without a whole new service.
//

#ifndef SOUL_SCENARIOSTATE_HPP
#define SOUL_SCENARIOSTATE_HPP

#include <fstream>
#include <string>
#include <string_view>

enum class ScenarioState
{
    Normal,
    SolarStormActive,
    Recovery
};

inline const char* ScenarioStateText(ScenarioState state)
{
    switch (state)
    {
    case ScenarioState::SolarStormActive:
        return "SolarStormActive";
    case ScenarioState::Recovery:
        return "Recovery";
    default:
        return "Normal";
    }
}

inline ScenarioState ScenarioStateFromText(std::string_view text)
{
    if (text == "SolarStormActive")
    {
        return ScenarioState::SolarStormActive;
    }

    if (text == "Recovery")
    {
        return ScenarioState::Recovery;
    }

    return ScenarioState::Normal;
}

inline std::string SharedScenarioPath()
{
    return "C:/SOUL/scenario_state.txt";
}

inline ScenarioState LoadSharedScenarioState()
{
    std::ifstream scenarioFile(SharedScenarioPath());
    if (!scenarioFile.is_open())
    {
        return ScenarioState::Normal;
    }

    std::string stateText;
    std::getline(scenarioFile, stateText);
    return ScenarioStateFromText(stateText);
}

inline bool SaveSharedScenarioState(ScenarioState state)
{
    std::ofstream scenarioFile(SharedScenarioPath(), std::ios::trunc);
    if (!scenarioFile.is_open())
    {
        return false;
    }

    scenarioFile << ScenarioStateText(state);
    return true;
}

#endif //SOUL_SCENARIOSTATE_HPP
