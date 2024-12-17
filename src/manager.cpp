// SPDX-License-Identifier: Apache-2.0

#include "manager.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>

namespace data_sync
{

Manager::Manager(const fs::path& dataSyncCfgDir)
{
    parseConfiguration(dataSyncCfgDir);
}

void Manager::parseConfiguration(const fs::path& dataSyncCfgDir)
{
    auto parse = [this](const auto& configFile) {
        try
        {
            std::ifstream file;
            file.open(configFile);

            nlohmann::json configJSON(nlohmann::json::parse(file));

            if (configJSON.contains("Files"))
            {
                std::ranges::transform(
                    configJSON["Files"],
                    std::back_inserter(this->_dataSyncConfiguration),
                    [](const auto& element) {
                    return config::DataSyncConfig(element);
                });
            }
            if (configJSON.contains("Directories"))
            {
                std::ranges::transform(
                    configJSON["Directories"],
                    std::back_inserter(this->_dataSyncConfiguration),
                    [](const auto& element) {
                    return config::DataSyncConfig(element);
                });
            }
        }
        catch (const std::exception& e)
        {
            // TODO Create error log
            lg2::error("Failed to parse the configuration file : {CONFIG_FILE}",
                       "CONFIG_FILE", configFile);
        }
    };

    if (fs::exists(dataSyncCfgDir) && fs::is_directory(dataSyncCfgDir))
    {
        std::ranges::for_each(
            fs::directory_iterator(dataSyncCfgDir),
            [&parse](const auto& configFile) { parse(fs::path(configFile)); });
    }

    printConfig();
}

void Manager::printConfig()
{
    for (auto element : _dataSyncConfiguration)
    {
        lg2::info("Path:{PATH} SyncDirection:{SYNC_DIRECTION} "
                  "SyncType:{SYNC_TYPE}",
                  "PATH", element._path, "SYNC_DIRECTION",
                  element.getSyncDirectionInStr(), "SYNC_TYPE",
                  element.getSyncTypeInStr());
        if (element._periodicityInSec.has_value())
        {
            lg2::info("PeriodicityInSec:{PERIODICITY}", "PERIODICITY",
                      element._periodicityInSec.value());
        }
        if (element._retry.has_value())
        {
            lg2::info(
                "RetryAttempts:{RETRY_ATTEMPTS} RetryIntervalInSec:{RETRY_INTERVAL}",
                "RETRY_ATTEMPTS", element._retry.value()._retryAttempts,
                "RETRY_INTERVAL", element._retry.value()._retryIntervalInSec);
        }
        if (element._excludeFileList.has_value())
        {
            std::string excludeList(std::ranges::fold_left(
                element._excludeFileList.value(), "ExcludeList",
                [](std::string_view items, std::string_view item) {
                std::string tmp(items);
                tmp.append(", ");
                tmp.append(item);
                return tmp;
            }));
            lg2::info("{EXCLUDE_LIST}", "EXCLUDE_LIST", excludeList);
        }
        if (element._includeFileList.has_value())
        {
            std::string includeList(std::ranges::fold_left(
                element._includeFileList.value(), "IncludeList",
                [](std::string_view items, std::string_view item) {
                std::string tmp(items);
                tmp.append(", ");
                tmp.append(item);
                return tmp;
            }));
            lg2::info("{INCLUDE_LIST}", "INCLUDE_LIST", includeList);
        }
    }
}

} // namespace data_sync
