DESCRIPTION = "ticos-device-info application"
LICENSE = "CLOSED"

S = "${WORKDIR}"

do_compile() {
    echo "#!/bin/sh" > ${S}/ticos-device-info
    echo "echo TICOS_DEVICE_ID=${TICOS_DEVICE_ID}" >> ${S}/ticos-device-info
    echo "echo TICOS_HARDWARE_VERSION=${TICOS_HARDWARE_VERSION}" >> ${S}/ticos-device-info
}

do_install() {
    install -Dm 755 ${S}/ticos-device-info ${D}/${bindir}/ticos-device-info
}
