#!/usr/bin/env python3

import os
import subprocess
import sys
import shutil
import pexpect

CA_DIR = "/etc/phosphor-data-sync/certs"
CERTS_DIR = "/etc/phosphor-data-sync/certs"
DAYS_VALID = 365
REMOTE_BMC1_IP = "10.2.2.100"
WAIT_BIN = "/usr/bin/wait_for_ca_files"
CA_KEY = os.path.join(CA_DIR, "ca.key")
CA_CRT = os.path.join(CA_DIR, "ca.crt")
BMC_POS_CMD = ["fw_printenv", "-n", "bmc_position"]
PASSWORD = "0penBmc0"


def run_cmd(cmd):
    subprocess.run(cmd, shell=True, check=True)


def scp_with_password(src_files, remote_ip, remote_path, password):
    scp_cmd = f"scp -o StrictHostKeyChecking=no {src_files} root@{remote_ip}:{remote_path}"
    print(f"[INFO] Running: {scp_cmd}")
    child = pexpect.spawn(scp_cmd, timeout=30)

    index = child.expect([
        r"[Pp]assword:",
        r"yes/no",
        pexpect.EOF,
        pexpect.TIMEOUT
    ])

    if index == 0:
        child.sendline(password)
        child.expect(pexpect.EOF)
    elif index == 1:
        child.sendline("yes")
        child.expect(r"[Pp]assword:")
        child.sendline(password)
        child.expect(pexpect.EOF)
    elif index == 2:
        print("[INFO] SCP completed.")
    elif index == 3:
        print("[ERROR] SCP timed out.")
        raise RuntimeError("SCP timeout")

    child.close()
    if child.exitstatus != 0:
        raise RuntimeError(f"SCP failed with exit code {child.exitstatus}")


def wait_for_ca_from_bmc0():
    print(f"[INFO] Waiting for CA from BMC0 using: {WAIT_BIN}")
    subprocess.run([WAIT_BIN], check=True)


def generate_cert(name, certs_dir, ca_dir, days_valid):
    key_path = os.path.join(certs_dir, f"{name}.key")
    csr_path = os.path.join(certs_dir, f"{name}.csr")
    crt_path = os.path.join(certs_dir, f"{name}.crt")

    if os.path.exists(crt_path) and os.path.exists(key_path):
        print(f"[INFO] Certificate already exists for {name}, skipping generation.")
        return

    print(f"[INFO] Generating certificate for {name}...")

    run_cmd(f"openssl genrsa -out {key_path} 2048")
    run_cmd(f"openssl req -new -key {key_path} -out {csr_path} "
            f"-subj \"/C=IN/ST=KA/L=Bangalore/O=IBM/OU=ISDL/CN={name}/emailAddress=powerfw@in.ibm.com\"")
    run_cmd(f"openssl x509 -req -in {csr_path} -CA {os.path.join(ca_dir, 'ca.crt')} "
            f"-CAkey {os.path.join(ca_dir, 'ca.key')} -CAcreateserial "
            f"-out {crt_path} -days {days_valid} -sha256")

    os.remove(csr_path)


def main():
    os.makedirs(CERTS_DIR, exist_ok=True)

    try:
        bmc_position = subprocess.check_output(BMC_POS_CMD, text=True).strip()
    except Exception as e:
        print(f"[ERROR] Unable to determine BMC position: {e}")
        sys.exit(1)

    if bmc_position == "bmc0":
        print("[INFO] This is BMC0.")

        if not os.path.exists(CA_KEY) or not os.path.exists(CA_CRT):
            print("[INFO] Generating CA certs...")
            run_cmd(f"openssl genrsa -out {CA_KEY} 2048")
            run_cmd(f"openssl req -x509 -new -nodes -key {CA_KEY} "
                    f"-sha256 -days {DAYS_VALID} -out {CA_CRT} "
                    f"-subj \"/C=IN/ST=KA/L=Bangalore/O=IBM/OU=ISDL/CN=rbmc/emailAddress=powerfw@in.ibm.com\"")
        else:
            print("[INFO] CA certs already exist, skipping generation.")

        print("[INFO] Transferring CA certs to BMC1...")
        scp_with_password(f"{CA_KEY} {CA_CRT}", REMOTE_BMC1_IP, CA_DIR, PASSWORD)

        generate_cert("bmc0", CERTS_DIR, CA_DIR, DAYS_VALID)

    elif bmc_position == "bmc1":
        print("[INFO] This is BMC1.")
        wait_for_ca_from_bmc0()
        generate_cert("bmc1", CERTS_DIR, CA_DIR, DAYS_VALID)

    else:
        print(f"[ERROR] Invalid BMC position: {bmc_position}")
        sys.exit(1)


if __name__ == "__main__":
    main()
