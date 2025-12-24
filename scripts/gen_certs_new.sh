#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# This script generates temporary certificates and a private key for the BMCs,
# which are used during data synchronization.

# If running on BMC0, this script generates the Certificate Authority (CA).
# If running on BMC1, it waits to receive the CA from BMC0.
# Based on the BMC's role, it generates the appropriate key and certificates.

# FIXME: This is a temporary workaround. Ideally, the certificates and private
# key should be generated and securely stored in the BMC-specific TPM during
# provisioning via SPDM. However, SPDM support is not yet available.

set -e

CA_DIR="/etc/phosphor-data-sync/certs"
CERTS_DIR="/etc/phosphor-data-sync/certs"
DAYS_VALID=365
BMC1_IP="10.2.2.100"
WAIT_FOR_CA="/usr/libexece/phosphor-data-sync/wait_for_ca_file"
CA_KEY="${CA_DIR}/ca.key"
CA_CRT="${CA_DIR}/ca.crt"
BMC_PASSWORD="0penBmc0"

generate_cert() {
    name="$1"
    key_path="${CERTS_DIR}/${name}.key"
    csr_path="${CERTS_DIR}/${name}.csr"
    crt_path="${CERTS_DIR}/${name}.crt"

    if [ -f "$key_path" ] && [ -f "$crt_path" ]; then
        echo "Certificate already exists for $name, skipping generation."
        return
    fi

    echo "Generating certificate for $name..."
    openssl genrsa -out "$key_path" 2048
    openssl req -new -key "$key_path" -out "$csr_path" \
        -subj "/C=IN/ST=KA/L=Bangalore/O=IBM/OU=ISDL/CN=$name/emailAddress=powerfw@in.ibm.com"
    openssl x509 -req -in "$csr_path" -CA "$CA_CRT" -CAkey "$CA_KEY" -CAcreateserial \
        -out "$crt_path" -days "$DAYS_VALID" -sha256

    rm -f "$csr_path"
}

transfer_ca() {
    src_files="$1"
    remote_ip="$2"
    remote_path="$3"
    password="$4"

    echo "Transferring: $src_files to $remote_ip:$remote_path"
    echo "$password" | sshpass -p "$password" scp -o StrictHostKeyChecking=no $src_files root@"$remote_ip":"$remote_path"
    if [ $? -ne 0 ]; then
        echo "Transfer is failed"
        exit 1
    fi
}

main() {
    mkdir -p "$CERTS_DIR"

    BMC_POSITION=$(fw_printenv -n bmc_position 2>/dev/null)

    if [ -z "$BMC_POSITION" ]; then
        echo "Unable to determine BMC position"
        exit 1
    fi

    if [ "$BMC_POSITION" = "bmc0" ]; then
        echo "This is BMC0"

        if [ ! -f "$CA_KEY" ] || [ ! -f "$CA_CRT" ]; then
            echo "Generating CA certs..."
            openssl genrsa -out "$CA_KEY" 2048
            openssl req -x509 -new -nodes -key "$CA_KEY" \
                -sha256 -days "$DAYS_VALID" -out "$CA_CRT" \
                -subj "/C=IN/ST=KA/L=Bangalore/O=IBM/OU=ISDL/CN=rbmc/emailAddress=powerfw@in.ibm.com"
        else
            echo "CA certs already exist, skipping generation."
        fi

        transfer_ca "$CA_KEY $CA_CRT" "$BMC1_IP" "$CA_DIR" "$BMC_PASSWORD"

        generate_cert "bmc0"

    elif [ "$BMC_POSITION" = "bmc1" ]; then
        echo "This is BMC1"
        echo "Waiting for CA from BMC0 using: $WAIT_FOR_CA"
        "$WAIT_FOR_CA"

        generate_cert "bmc1"

    else
        echo "Invalid BMC position: $BMC_POSITION"
        exit 1
    fi
}

main
