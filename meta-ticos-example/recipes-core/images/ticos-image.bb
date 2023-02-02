DESCRIPTION = "Main Ticos wrapper target build"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit image

# Disable most of the image build process
do_rootfs[noexec] = "1"
do_rootfs_wicenv[noexec] = "1"
do_image[noexec] = "1"
do_image_wic[noexec] = "1"
do_image_ext4[noexec] = "1"
do_image_tar[noexec] = "1"
do_image_complete_setscene[noexec] = "1"
do_image_qa[noexec] = "1"
do_image_qa_setscene[noexec] = "1"
do_build[noexec] = "1"

# Overload instead of disabling to allow dependencies to be processed correctly
python do_image_complete() {
    pass
}

do_image[depends] += "swupdate-image:do_swuimage base-image:do_image_complete"
