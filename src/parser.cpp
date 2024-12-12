// SPDX-License-Identifier: Apache-2.0

#include "parser.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <string>

// ToDo : namespace

namespace fs = std::filesystem;

Retry::Retry(const int& retryAttempts, const uint16_t& retryIntervalInSec) :
    _retryAttempts(retryAttempts), _retryIntervalInSec(retryIntervalInSec)
{}

SyncDataInfo::SyncDataInfo(const nlohmann::json& json) :
    _path(to_string(json["Path"])),
    _syncDirection(parseSyncDirection(json["SyncDirection"])),
    _syncType(parseSyncType(json["SyncType"]))
{
    // Initiailze optional members
    if (_syncType == SyncType::Periodic)
    {
        _periodicityInSec = parseISODurationToSec(json["Periodicity"]);
    }
    else
    {
        _periodicityInSec = std::nullopt;
    }

    if (json.contains("RetryAttempts") && json.contains("RetryInterval"))
    {
        _retry = Retry(json["RetryAttempts"],
                       parseISODurationToSec(json["RetryInterval"]));
    }
    else
    {
        _retry = std::nullopt;
    }

    if (json.contains("ExcludeFilesList"))
    {
        _excludeFileList = json["ExcludeFilesList"];
    }
    else
    {
        _excludeFileList = std::nullopt;
    }

    if (json.contains("IncludeFilesList"))
    {
        _includeFileList = json["IncludeFilesList"];
    }
    else
    {
        _includeFileList = std::nullopt;
    }
}

SyncDataConfig::SyncDataConfig()
{
    // TB complete
    lg2::info(
        "Constructor of SyncDataConfig invoked and attempting to read JSON files");

    fs::path configFilesDir{DATA_SYNC_CONFIG_DIR};

    if (fs::exists(configFilesDir) && fs::is_directory(configFilesDir))
    {
        std::ranges::for_each(
            fs::directory_iterator(configFilesDir),
            [this](const auto& configFile) { parseConfig(configFile); });
    }
}

/*SyncDataConfig* SyncDataConfig::getConfiguration()
{
    if(syncDataConfigPtr == nullptr)
    {
        syncDataConfigPtr = new SyncDataConfig();
    }
    return syncDataConfigPtr;
}
*/
void SyncDataConfig::parseConfig(const std::filesystem::path& configFileName)
{
    try
    {
        nlohmann::json configJSON;

        std::ifstream file;
        file.open(configFileName);

        configJSON = nlohmann::json::parse(file);
        if (configJSON.contains("Files"))
        {
            std::ranges::transform(
                configJSON["Files"], std::back_inserter(_syncDataInfo),
                [](const auto& element) { return SyncDataInfo(element); });
        }
        if (configJSON.contains("Directories"))
        {
            std::ranges::transform(
                configJSON["Directories"], std::back_inserter(_syncDataInfo),
                [](const auto& element) { return SyncDataInfo(element); });
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to parse the configuration file : {CONFIG_FILE}",
                   "CONFIG_FILE", configFileName);
        // ToDo : Create an error log here.
    }
}

// helper APIs

SyncDirection parseSyncDirection(const std::string& syncDirectionValue)
{
    SyncDirection syncDirection = SyncDirection::Bidirectional;
    if (syncDirectionValue == "Active2Passive")
    {
        syncDirection = SyncDirection::Active2Passive;
    }
    else if (syncDirectionValue == "Passive2Active")
    {
        syncDirection = SyncDirection::Passive2Active;
    }

    return syncDirection;
}

SyncType parseSyncType(const std::string& syncTypeValue)
{
    SyncType syncType = SyncType::Immediate;
    if (syncTypeValue == "Immediate")
    {
        syncType = SyncType::Immediate;
    }
    else if (syncTypeValue == "Periodic")
    {
        syncType = SyncType::Periodic;
    }

    return syncType;
}

uint16_t parseISODurationToSec(const std::string& timeIntervalInISO)
{
    // ToDo: Parse periodicity in ISO 8601 duration format using chrono
    std::smatch match;
    std::regex isoDurationRegex("PT(([0-9]+)H)?(([0-9]+)M)?(([0-9]+)S)?");

    if (std::regex_search(timeIntervalInISO, match, isoDurationRegex))
    {
        return (
            (match.str(2).empty() ? 0 : (std::stoi(match.str(2)) * 60 * 60)) +
            (match.str(4).empty() ? 0 : (std::stoi(match.str(4)) * 60)) +
            (match.str(6).empty() ? 0 : std::stoi(match.str(6))));
    }
    else
    {
        lg2::error("{TIME} is not matching with ISO 8601 duration format",
                   "TIME", timeIntervalInISO);
        return 0;
    }
}

void SyncDataConfig::displayConfig()
{
    for (auto element : _syncDataInfo)
    {
        std::cout << "Path : " << element._path << std::endl;
        // std::cout << "SyncDirection : " << element._syncDirection <<
        // std::endl; std::cout << "SyncType : " << element._syncType <<
        // std::endl;
        if (element._periodicityInSec.has_value())
        {
            std::cout << "Periodicity in secs : "
                      << element._periodicityInSec.value() << std::endl;
        }
        else
        {
            std::cout << "Periodicity in secs : Not applicable" << std::endl;
        }
    }
}
