// SPDX-License-Identifier: Apache-2.0

#include "config.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>

// ToDo : namespace

/**
 * @brief The enum contains all the sync directions.
 */
enum class SyncDirection
{
    Active2Passive, // Sync from Active to Passive
    Passive2Active, // Sync from Passive to Active
    Bidirectional   // Sync from both Active to Passive as well as
                    // from Passive to Active
};

/**
 * @brief The enum contains all the sync types.
 */
enum class SyncType
{
    Immediate, // Sync immediately once inotify event receives for the data
    Periodic   // Sync periodically based on the periodicity value.
};

/**
 * @brief The structure containing all the details related to retry specific
 *        to a file/directory to be synced.
 */
struct Retry
{
    /**
     * Integer which represents the number of retries.
     */
    int _retryAttempts;

    /**
     * Integer which represents the time interval between the retries after
     * converting to seconds from ISO 8601 duration format.
     */
    uint16_t _retryIntervalInSec;

    /**
     * @brief The constructor
     *
     * @param[in] retryAttempts - The number of retries
     * @param[in] retryIntervalInSec - The interval in which retry will occur.
     */
    Retry(const int& retryAttempts, const uint16_t& retryIntervalInSec);
};

/**
 * @brief The structure represents each JSON object in the data sync
 *        configuration file which represents the file/directory to be
 *        synced between BMCs.
 */
struct SyncDataInfo
{
    /**
     * The string which represents the 'Path' in the JSON config file.
     */
    std::string _path;

    /**
     * The enum of type SyncDirection to represents the 'SyncDirection' field in
     * the JSON config file.
     */
    SyncDirection _syncDirection;

    /**
     * The enum of type SyncType to represents the 'SyncType' in the JSON config
     * file.
     */
    SyncType _syncType;

    /**
     * Integer to hold the 'Periodicity' in seconds after converting from ISO
     * 8601 duration format.
     */
    std::optional<uint16_t> _periodicityInSec;

    /**
     * Object of the struct Retry which is optional and has the JSON field
     * values 'RetryAttempts' and 'RetryInterval'.
     */
    std::optional<Retry> _retry;

    /**
     * Vector of string to hold the list of files to be excluded for sync.
     */
    std::optional<std::vector<std::string>> _excludeFileList;

    /**
     * Vector of string to hold the list of files to be included for sync.
     */
    std::optional<std::vector<std::string>> _includeFileList;

    /**
     * @brief The constructor
     *
     * @param[in] - json - The JSON object as defined in the JSON config file.
     */
    SyncDataInfo(const nlohmann::json& json);
};

/**
 * @class SyncDataConfig
 *
 * This class has the cotainer which holds the data sync configuration after
 * parsing the JSON configuration files inside 'config/data_sync_list'.
 *
 */
class SyncDataConfig
{
  public:
    ~SyncDataConfig() = default;
    SyncDataConfig(const SyncDataConfig&) = default;
    SyncDataConfig& operator=(const SyncDataConfig&) = default;
    SyncDataConfig(SyncDataConfig&&) = default;
    SyncDataConfig& operator=(SyncDataConfig&&) = default;

    /**
     * @brief The constructor
     *
     * The constructor will get invoked from Manager class.
     */
    SyncDataConfig();

    // TBRemoved
    void displayConfig();

    // Manager can access the _syncDataInfo
    friend class Manager;

  private:
    /**
     * @brief The API to parse the JSON objects defined in the JSON config
     * files.
     *
     * @param[in] - configFileName - Path where all the JSON config files
     *                               are placed.
     */
    void parseConfig(const std::filesystem::path& configFileName);

    /**
     * Container of the type `SyncDataInfo` to store all the parsed JSON
     * objects.
     */
    std::vector<SyncDataInfo> _syncDataInfo;
};

// Helper APIs

/**
 * @brief The API to convert string value of syncDirection to enum of type
 * SyncDirection.
 *
 * @param[in] - syncDirection - Value of the JSON field 'SyncDirection' in
 *                              string format.
 *
 * @returns - enum value of type SyncDirection.
 */
SyncDirection parseSyncDirection(const std::string& syncDirectionValue);

/**
 * @brief The API to convert string value of syncType to enum of type
 * SyncType.
 *
 * @param[in] - syncType - Value of the JSON field 'SyncType' in
 *                         string format.
 *
 * @returns - enum value of type SyncType.
 */

SyncType parseSyncType(const std::string& syncTypeValue);

/**
 * @brief API to convert the time duration in ISO 8601 duration format into
 * seconds.
 *
 * @param[in] - timeIntervalInISO - The time duration in ISO 8601 duration
 *                                  format.
 *
 * @returns - Time interval in seconds in uint16_t format.
 */
uint16_t parseISODurationToSec(const std::string& timeIntervalInISO);

/* To Be Removed */
void displayConfig(std::vector<SyncDataInfo> syncDataConfig);
