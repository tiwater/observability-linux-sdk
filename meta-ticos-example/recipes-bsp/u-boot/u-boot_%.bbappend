FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://0001-env-in-fat-defconfig-${PV}.patch \
    file://0002-initr_env-delay-${PV}.patch \
    file://0003-ticos_boot-boot-commands.patch \
    file://fw_env.config \
"
