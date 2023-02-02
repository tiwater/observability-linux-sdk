FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://09-swupdate-args.in \
    file://swupdate.cfg \
    file://defconfig \
"

do_swupdate_args_update() {
    # Yocto dependency checking can be broken if we modify the source file
    # directly during the build process, create a 'output' file to modify
    cp ${WORKDIR}/09-swupdate-args.in ${WORKDIR}/09-swupdate-args
    sed -i -e "s%__TICOS_HARDWARE_VERSION%${TICOS_HARDWARE_VERSION}%" ${WORKDIR}/09-swupdate-args
}
addtask do_swupdate_args_update before do_install after do_unpack


do_install:append() {
    install -Dm 0644 ${WORKDIR}/09-swupdate-args ${D}${libdir}/swupdate/conf.d/09-swupdate-args
    install -Dm 0644 ${WORKDIR}/swupdate.cfg ${D}${sysconfdir}/swupdate.cfg
}
