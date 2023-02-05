#!/bin/bash -exu

# YOCTO_RELEASE environment variable is expected to be set to i.e. "kirkstone", "dunfell", ...

poky_dir="${HOME}/yocto/sources/poky"
if [ ! -d "${poky_dir}" ]; then
    git clone https://git.yoctoproject.org/git/poky --branch "${YOCTO_RELEASE}" "${poky_dir}"
else
    git -C "${poky_dir}" checkout "${YOCTO_RELEASE}" && git -C "${poky_dir}" pull --ff-only
fi

openembedded_dir="${HOME}/yocto/sources/meta-openembedded"
if [ ! -d "${openembedded_dir}" ]; then
    git clone https://github.com/openembedded/meta-openembedded.git --branch "${YOCTO_RELEASE}" "${openembedded_dir}"
else
    git -C "${openembedded_dir}" checkout "${YOCTO_RELEASE}" && git -C "${openembedded_dir}" pull --ff-only
fi

swupdate_dir="${HOME}/yocto/sources/meta-swupdate"
if [ ! -d "${swupdate_dir}" ]; then
    git clone https://github.com/sbabic/meta-swupdate.git --branch "${YOCTO_RELEASE}" "${swupdate_dir}"
else
    git -C "${swupdate_dir}" checkout "${YOCTO_RELEASE}" && git -C "${swupdate_dir}" pull --ff-only
fi

# oe-init-build-env requires allowing unbound variables...:
set +u

cd "${HOME}/yocto"
TEMPLATECONF=../eticos-linux-sdk/meta-ticos-example/conf/ source "${HOME}/yocto/sources/poky/oe-init-build-env" build

# run any args given to us (defaults to Dockerfile's CMD)
exec "$@"
