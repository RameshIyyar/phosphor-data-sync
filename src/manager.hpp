// SPDX-License-Identifier: Apache-2.0

#include "parser.hpp"

#include <phosphor-logging/lg2.hpp>

#include <memory>

class Manager
{
  public:
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = default;

    Manager() : _syncDataConfig(std::make_unique<SyncDataConfig>()) {}

    /*
     *API to start the periodic sync
     */
    void startPeriodicSync();

  private:
    /**
     * @brief The unique pointer to the SyncDataConfig class object.
     */
    std::unique_ptr<SyncDataConfig> _syncDataConfig;
};
