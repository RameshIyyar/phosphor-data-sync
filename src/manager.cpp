// SPDX-License-Identifier: Apache-2.0

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

#include <iostream>

void Manager::startPeriodicSync()
{
    lg2::info("Parsing completed and data is available in the container");
    for (const auto& element : _syncDataConfig->_syncDataInfo)
    {
        lg2::info("Path of the file/directory : {PATH}", "PATH", element._path);
    }
}
