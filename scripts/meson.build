# SPDX-License-Identifier: Apache-2.0

# Install script to update sibling bmc ip address
install_data(
    'update_sibling_bmc_ip.sh',
    install_dir: libexecdir_installdir,
    install_mode: 'rwxr-xr-x',
)

# Generate CA, certificates and keys for the mTLS
generated_certs = custom_target(
  'generate_certs',
  output: [
    'ca.crt',
    'bmc0.crt',
    'bmc0.key',
    'bmc1.crt',
    'bmc1.key'
  ],
  command: [files('gen_certs.sh'), meson.current_build_dir()],
  build_by_default: true,
  install: true,
  install_dir: join_paths(etc_dir, 'certs')
)
