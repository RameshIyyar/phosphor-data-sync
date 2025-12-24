// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "external_data_ifaces.hpp"

#include <sdbusplus/async.hpp>

namespace data_sync::ext_data
{

/**
 * @class ExternalDataIFacesImpl
 *
 * @brief This class inherits from ExternalDataIFaces to implement
 *        the defined interfaces, enabling seamless use of dependent data
 *        managed by different applications.
 */
class ExternalDataIFacesImpl : public ExternalDataIFaces
{
  public:
    ExternalDataIFacesImpl(const ExternalDataIFacesImpl&) = delete;
    ExternalDataIFacesImpl& operator=(const ExternalDataIFacesImpl&) = delete;
    ExternalDataIFacesImpl(ExternalDataIFacesImpl&&) = delete;
    ExternalDataIFacesImpl& operator=(ExternalDataIFacesImpl&&) = delete;
    ~ExternalDataIFacesImpl() override = default;

    /**
     * @brief The constructor to fetch all external dependent data's
     *
     * @param[in] ctx - The async context
     */
    explicit ExternalDataIFacesImpl(sdbusplus::async::context& ctx);

    /**
     * @brief Used to start the sync service (rsync and stunnel).
     *
     * The rsync service will take care to start stunnel service.
     *
     * @return True if the sync service started successfully otherwise false.
     */
    sdbusplus::async::task<bool> startSyncService() override;

    /**
     * @brief Used to wait for sync service (rsync and stunnel) to start.
     *
     * The rsync service will wait for the stunnel service to start as well.
     *
     * @param[in] path - The service object path to check state
     *
     * @return True if the sync service started successfully otherwise false.
     */
    sdbusplus::async::task<bool> waitForSyncService(const sdbusplus::message::object_path& path = sdbusplus::message::object_path()) override;

    /**
     * @brief Check for BMC redundancy to be enabled and start sync service.
     *
     * The BMC role will be updated if its changed.
     */
    sdbusplus::async::task<> checkBMCRedundancyAndStartSyncServ() override;

    /**
     * @brief Used to wait for the sibling BMC to get ready to initiate sync
     *        operation.
     *
     * @return True if the sibling BMC is ready to sync otherwise false.
     */
    sdbusplus::async::task<bool> waitForSiblingBMCReadyTosync() override;

  private:
    /**
     * @brief Utility API to get the DBus service name of the given
     *        object path and interface.
     *
     * @param[in] objPath - The object path
     * @param[in] interface - The DBus interface name
     *
     * @return The service name
     */
    sdbusplus::async::task<std::string>
        getDBusService(const std::string& objPath,
                       const std::string& interface);

    /**
     * @brief  Used to retrieve the BMC role from DBus.
     */
    sdbusplus::async::task<> fetchBMCRedundancyMgrProps() override;

    /**
     * @brief Used to retrieve the BMC Position from Dbus.
     */
    sdbusplus::async::task<> fetchBMCPosition() override;

    /**
     * @brief Notifies the sibling BMC via the RBMC manager that it can
     *        initiate synchronization.
     *
     * @param[in] readyToSync Indicates whether synchronization can be
     *                        initiated.
     */
    sdbusplus::async::task<> informSiblingBMCToStartSync(const RBMC::ReadyToSync readyToSync);

    /**
     * @brief Used to get the async context
     */
    sdbusplus::async::context& _ctx;
};

} // namespace data_sync::ext_data
