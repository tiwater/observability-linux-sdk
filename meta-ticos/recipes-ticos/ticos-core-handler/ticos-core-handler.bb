DESCRIPTION = "ticos-core-handler application"
LICENSE = "Proprietary"
LICENSE_FLAGS = "commercial"
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../License.txt;md5=f10c502d265f86bd71f9dac8ec7827c2"

SRC_URI = " \
    file://ticos-core-handler \
"

S = "${WORKDIR}/ticos-core-handler"

inherit cmake
