#!/bin/sh

TUN=sl0
PREFIX=128
TUN_GLB="2001::200:0:cafe" # Dirección global de prueba
SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")

SUDO=${SUDO:-sudo}

unsupported_platform() {
    echo "unsupported platform" >&2
    echo "(currently supported \`uname -s\` 'Darvin' and 'Linux')" >&2
}

case "$(uname -s)" in
    Darvin)
        PLATFORM="OSX";;
    Linux)
        PLATFORM="Linux";;
    *)
        unsupported_platform
        exit 1
        ;;
esac

add_addresses() {
    case "${PLATFORM}" in
        Linux)
            cargo build --release
            ${SUDO} ip address add ${TUN_GLB} dev ${TUN}
            ${SUDO} target/release/vaina rcs add ${TUN} ${TUN_GLB}
            ${SUDO} target/release/vaina nib add ${TUN} ${PREFIX} ${TUN_GLB}
            ;;
        OSX)
# TODO: add the IPV6 address to the interface!!!
            ${SUDO} target/release/vaina rcs add ${TUN} ${TUN_GLB}
            ${SUDO} target/release/vaina nib add ${TUN} ${PREFIX} ${TUN_GLB}
            unsupported_platform
            ;;
    esac
    return 0
}

add_addresses
