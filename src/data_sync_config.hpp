// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace data_sync
{
namespace config
{

/**
 * @brief The enum contains all the sync directions.
 */
enum class SyncDirection
{
    Active2Passive,
    Passive2Active,
    Bidirectional
};

/**
 * @brief The enum contains all the sync types.
 */
enum class SyncType
{
    Immediate,
    Periodic
};

/**
 * @brief The structure contains all retry-related details
 *        specific to a file or directory to retry if failed to sync.
 */
struct Retry
{
    /**
     * @brief The constructor
     *
     * @param[in] retryAttempts - The number of retries
     * @param[in] retryIntervalInSec - The interval in which retry will
     *                                 occur.
     */
    Retry(const int& retryAttempts, const uint16_t& retryIntervalInSec);

    /**
     * @brief Number of retries.
     */
    int _retryAttempts;

    /**
     * @brief Retry interval in seconds
     */
    uint16_t _retryIntervalInSec;
};

/**
 * @brief The structure contains data sync configuration specified
 *        in the configuration file for each file or directory to be
 *        synchronized between BMCs.
 */
struct DataSyncConfig
{
  public:
    /**
     * @brief The constructor initializes members using the configuration.
     *
     * @param[in] config - The sync data information
     */
    DataSyncConfig(const nlohmann::json& config);

    /**
     * @brief Get sync direction in string format.
     *
     * @return The sync direction in string
     */
    constexpr std::string_view getSyncDirectionInStr() const
    {
        switch (_syncDirection)
        {
            case SyncDirection::Active2Passive:
                return "Active2Passive";
            case SyncDirection::Passive2Active:
                return "Passive2Active";
            case SyncDirection::Bidirectional:
                return "Bidirectional";
        }
        return "";
    }

    /**
     * @brief Get sync type in string format.
     *
     * @return The sync type in string
     */
    constexpr std::string_view getSyncTypeInStr() const
    {
        switch (_syncType)
        {
            case SyncType::Immediate:
                return "Immediate";
            case SyncType::Periodic:
                return "Periodic";
        }
        return "";
    }

    /**
     * @brief The file or directory path to be synchronized.
     */
    std::string _path;

    /**
     * @brief Used to get sync direction.
     */
    SyncDirection _syncDirection;

    /**
     * @brief Used to get sync type.
     */
    SyncType _syncType;

    /**
     * @brief The interval (in seconds) to sync periodically.
     *
     * @note Holds a value if the synchronization type is set to Periodic.
     */
    std::optional<uint16_t> _periodicityInSec;

    /**
     * @brief The Retry specific details.
     *
     * @note Holds a value if the specific file or directory uses
     *       a custom retry preference.
     */
    std::optional<Retry> _retry;

    /**
     * @brief The list of paths to exclude from synchronization.
     *
     * @note Holds a value if the specific directory prefer to
     *       exclude some file from synchronization.
     */
    std::optional<std::vector<std::string>> _excludeFileList;

    /**
     * @brief The list of paths to include from synchronization.
     *
     * @note Holds a value if the specific directory opts to
     *       include only certain file during the synchronization.
     */
    std::optional<std::vector<std::string>> _includeFileList;

  private:
    /**
     * @brief A helper API to retrieve the corresponding enum type
     *        for a given sync direction string.
     *
     * @param[in] syncDirection - the sync direction
     *
     * @returns The enum value on success; otherwise, nullopt.
     */
    static std::optional<SyncDirection>
        convertSyncDirectionToEnum(const std::string& syncDirection);

    /**
     * @brief A helper API to retrieve the corresponding enum type
     *        for a given sync type string.
     *
     * @param[in] - syncType - the sync type
     *
     * @returns The enum value on success; otherwise, nullopt.
     */
    static std::optional<SyncType>
        convertSyncTypeToEnum(const std::string& syncType);

    /**
     * @brief A helper API to convert the time duration in ISO 8601 duration
     *        format into seconds
     *
     * @param[in] - timeIntervalInISO - The time duration
     *
     * @returns The time interval in seconds on success; otherwise, nullopt.
     */
    static std::optional<uint16_t>
        convertISODurationToSec(const std::string& timeIntervalInISO);
};

} // namespace config
} // namespace data_sync