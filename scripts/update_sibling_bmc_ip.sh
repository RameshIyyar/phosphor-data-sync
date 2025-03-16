#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

# This script updates the sibling BMC IP in the corresponding data sync stunnel
# configuration file based on the sibling BMC's position.

set -e

# Note: It's temporary until the corresponding daemon hosts
#       the local BMC position.
localBMCPos=$(fw_printenv -n bmc_position)
siblingBMCIPAddr=""

# Note: It's temporary until the networkd hosts the sibling BMC IP address.
if [ "$localBMCPos" = 0 ]
then
    # Using the simics bmc1 eth0 IP address
    siblingBMCIPAddr="10.2.2.100"
elif [ "$localBMCPos" = 1 ]
then
    # Using the simics bmc0 eth0 IP address
    siblingBMCIPAddr="10.0.2.100"
else
    echo "Unsupported [$localBMCPos] local bmc position"
    exit 1
fi

# The below sed command modifies the first "connect = " line inside
# the "[sibling_bmc]" service section, replacing the IP address and stopping
# further modifications after that. It also stops processing when encountering
# a new service section ([something]).
# In the stunnel configuration, each service section begins with "[" and
# continues until another section header ("[") is found.
sed -i '/sibling_bmc/ {n; :loop; /^\[/ b end; \
        /connect = / {s/\(connect = \)[^:]*\(.*\)/\1'$siblingBMCIPAddr'\2/; \
    b end;}; n; b loop; :end;}' \
    /etc/phosphor-data-sync/stunnel/bmc"$localBMCPos"_stunnel.conf
