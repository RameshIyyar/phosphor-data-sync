// SPDX-License-Identifier: Apache-2.0

#include "external_data_ifaces_impl.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Inventory/Decorator/Position/client.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>
#include <xyz/openbmc_project/State/BMC/Redundancy/client.hpp>

namespace data_sync::ext_data
{

constexpr auto rsyncServ = "SyncBMCData_rsync.service";

namespace service
{
constexpr auto systemd = "org.freedesktop.systemd1";
} // namespace service

namespace interface
{
constexpr auto systemdMgr = "org.freedesktop.systemd1.Manager";
constexpr auto systemdUnit = "org.freedesktop.systemd1.Unit";
} // namespace interface

namespace object_path
{
constexpr auto systemd = "/org/freedesktop/systemd1";
} // namespace object_path

ExternalDataIFacesImpl::ExternalDataIFacesImpl(sdbusplus::async::context& ctx) :
    _ctx(ctx)
{}

sdbusplus::async::task<std::string>
    // NOLINTNEXTLINE
    ExternalDataIFacesImpl::getDBusService(const std::string& objPath,
                                           const std::string& interface)
{
    try
    {
        using ObjectMapperMgr =
            sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

        auto objectMapperMgr = ObjectMapperMgr(_ctx)
                                   .service(ObjectMapperMgr::default_service)
                                   .path(ObjectMapperMgr::instance_path);

        std::vector<std::string> interfaces{interface};

        auto services = co_await objectMapperMgr.get_object(objPath,
                                                            interfaces);

        co_return services.begin()->first;
    }
    catch (const std::exception& e)
    {
        lg2::error("D-Bus error [{ERROR}] while trying to get service name for "
                   "ObjectPath: {OBJ_PATH} Interface: {IFACE}",
                   "ERROR", e, "OBJ_PATH", objPath, "IFACE", interface);
        throw;
    }
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ExternalDataIFacesImpl::fetchBMCRedundancyMgrProps()
{
    try
    {
        auto rbmcMgr = sdbusplus::async::proxy()
                           .service(RBMC::interface)
                           .path(RBMC::instance_path)
                           .interface(RBMC::interface);

        auto props =
            co_await rbmcMgr.get_all_properties<RBMC::PropertiesVariant>(_ctx);

        auto it = props.find("Role");
        if (it != props.end())
        {
            bmcRole(std::get<BMCRole>(it->second));
        }

        it = props.find("RedundancyEnabled");
        if (it != props.end())
        {
            bmcRedundancy(std::get<BMCRedundancy>(it->second));
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get the RBMC properties, error: {ERROR}", "ERROR",
                   e);
        throw;
    }
    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> ExternalDataIFacesImpl::fetchBMCPosition()
{
    try
    {
        // In a redundant BMC system, the local BMC position is maintained
        // in the system inventory.
        using PositionMgr = sdbusplus::client::xyz::openbmc_project::inventory::
            decorator::Position<>;

        const auto* const systemInvObjPath =
            "/xyz/openbmc_project/inventory/system";

        auto service = co_await getDBusService(systemInvObjPath,
                                               PositionMgr::interface);

        bmcPosition(co_await PositionMgr(_ctx)
                        .service(service)
                        .path(systemInvObjPath)
                        .position());
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get the BMC position, error: {ERROR}", "ERROR",
                   e);
        throw;
    }
    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<bool> ExternalDataIFacesImpl::startSyncService()
{
    /**
     * Using Restart to ensure the service restarts even if already running.
     * This handles cases where the sibling BMC IP address has changed.
     * Restarting the rsync service also triggers the stunnel service,
     * as it is managed as part of the rsync service.
     */
    co_await informSiblingBMCToStartSync(RBMC::ReadyToSync::AwaitingReadiness);


    constexpr auto systemdMgr = sdbusplus::async::proxy()
                                    .service(service::systemd)
                                    .path(object_path::systemd)
                                    .interface(interface::systemdMgr);

    auto objPath = co_await systemdMgr.call<sdbusplus::message::object_path>(
            _ctx, "RestartUnit", rsyncServ, std::string{"replace"});

    auto started = co_await waitForSyncService(objPath);

    co_await informSiblingBMCToStartSync(started ? RBMC::ReadyToSync::Yes : RBMC::ReadyToSync::No);

    co_return started;
}

// NOLINTNEXTLINE
sdbusplus::async::task<bool> ExternalDataIFacesImpl::waitForSyncService(const sdbusplus::message::object_path& path)
{
    auto objPath = path;

    if (path.str.empty())
    {
        constexpr auto systemdMgr = sdbusplus::async::proxy()
            .service(service::systemd)
            .path(object_path::systemd)
            .interface(interface::systemdMgr);

        objPath = co_await systemdMgr.call<sdbusplus::message::object_path>(
                    _ctx, "GetUnit", rsyncServ);
    }

    lg2::info("Object Path: {OBJ_PATH}", "OBJ_PATH", objPath.str);

    auto rsyncServMgr = sdbusplus::async::proxy()
                            .service(service::systemd)
                            .path(objPath.str)
                            .interface(interface::systemdUnit);

    std::string state;
    while (true)
    {
        state = co_await rsyncServMgr.get_property<std::string>(_ctx, "ActiveState");

        // Check for known stable systemd service states
        if (state == "active" || state == "failed" || state == "inactive")
        {
            break;
        }

        lg2::debug("Sync service state: {SERV_STATE}", "SERV_STATE", state);
        // Wait for a short period before checking again
        using namespace std::chrono_literals;
        co_await sdbusplus::async::sleep_for(_ctx, 500ms);
    }

    if (state != "active")
    {
        // TODO Create error log if failed to start sync service and exit?
        lg2::error("Failed to start sync service and sync cannot be done");
        co_return false;
    }
    co_return true;
}

sdbusplus::async::task<>
    // NOLINTNEXTLINE
    ExternalDataIFacesImpl::checkBMCRedundancyAndStartSyncServ()
{
    sdbusplus::async::match match(_ctx,
            sdbusplus::bus::match::rules::propertiesChanged(RBMC::instance_path, RBMC::interface));

    using PropertyMap = std::map<std::string, RBMC::PropertiesVariant>;

    while (!_ctx.stop_requested())
    {
        auto [_, props] = co_await match.next<std::string, PropertyMap>();

        auto it = props.find("Role");
        if (it != props.end())
        {
            bmcRole(std::get<BMCRole>(it->second));
        }

        it = props.find("RedundancyEnabled");
        if (it != props.end())
        {
            bmcRedundancy(std::get<BMCRedundancy>(it->second));
            if (bmcRedundancy())
            {
                lg2::info("BMC redundancy is enabled, start sync service.");
                co_await startSyncService();
            }
        }
    }
    co_return;
}

sdbusplus::async::task<>
    // NOLINTNEXTLINE
    ExternalDataIFacesImpl::informSiblingBMCToStartSync(const RBMC::ReadyToSync readyToSync)
{
    using RMCMgr = sdbusplus::client::xyz::openbmc_project::state::bmc::
        Redundancy<>;

    auto rbmcMgr = RMCMgr(_ctx)
        .service(RBMC::interface)
        .path(RBMC::instance_path);

    co_await rbmcMgr.ready_to_sync(readyToSync);
    co_return;
}

sdbusplus::async::task<bool>
    // NOLINTNEXTLINE
    ExternalDataIFacesImpl::waitForSiblingBMCReadyTosync()
{
    sdbusplus::async::match match(_ctx,
            sdbusplus::bus::match::rules::propertiesChanged(
                std::format("{}/{}", RBMC::namespace_path::value, RBMC::namespace_path::sibling_bmc),
                RBMC::interface));

    using PropertyMap = std::map<std::string, RBMC::PropertiesVariant>;
    while (!_ctx.stop_requested())
    {
        auto [_, props] = co_await match.next<std::string, PropertyMap>();

        auto it = props.find("ReadyToSync");
        if (it != props.end())
        {
            lg2::debug("Sibling ReadyToSync is changed: {READY_TO_SYNC}",
                       "READY_TO_SYNC", RBMC::convertReadyToSyncToString(std::get<SiblingReadyToSync>(it->second)));
            if (std::get<SiblingReadyToSync>(it->second) == SiblingReadyToSync::AwaitingReadiness)
            {
                continue;
            }
            else
            {
                co_return std::get<SiblingReadyToSync>(it->second) == SiblingReadyToSync::Yes ? true : false;
            }
        }
    }

    co_return false;
}

} // namespace data_sync::ext_data
