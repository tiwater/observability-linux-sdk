DESCRIPTION = "ticos-core-handler application"
LICENSE = "Proprietary"
LICENSE_FLAGS = "commercial"
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../License.txt;md5=2642e777178e4d35bc731713bb5b2279"

SRC_URI = " \
    file://ticos-core-handler \
"

S = "${WORKDIR}/ticos-core-handler"

inherit cmake
