FROM ubuntu:latest

# **NOTE: DO NOT PUT ANYTHING SECRET / CONFIDENTIAL IN THE DOCKER IMAGE** The
# image is publically published.

ENV DEBIAN_FRONTEND noninteractive
ENV YOCTO_RELEASE ${YOCTO_RELEASE}

RUN apt-get update && \
    apt-get -y upgrade

# Core utilities
RUN apt-get install -y bash build-essential curl git sudo vim

# Yocto dependencies
RUN apt-get install -y chrpath cpio diffstat gawk file lz4 python3-distutils wget zstd

# Yocto QEMU dependencies
RUN apt-get install -y samba

# Yocto wic dependencies
RUN apt-get install -y parted

# Install pip for python testing dependencies
RUN apt-get install -y python3-pip

# CppUnit testing dependencies for ticosd's unit tests
RUN apt-get install -y \
    cmake \
    libcurl4-openssl-dev \
    libjson-c-dev \
    libsystemd-dev \
    libubootenv-dev \
    uuid-dev \
    pkg-config

# On Intel hardware only: Compile and install cpputest (32bit/64bit) and its dependencies
COPY install-cpputest.sh /
RUN test "$(uname -m)" = "x86_64" && /install-cpputest.sh && rm -rf /install-cpputest.sh || /bin/true

# Add normal user
RUN useradd --create-home --gid users --groups sudo --shell /bin/bash build
RUN sed -i -e 's/^\(%sudo[[:space:]].*\)ALL$/\1NOPASSWD: ALL/' /etc/sudoers

# Add en_US.UTF-8 locale, required by Yocto
RUN apt-get install -y locales
RUN sed -i -e 's/^# en_US.UTF-8 UTF-8$/en_US.UTF-8 UTF-8/' /etc/locale.gen && echo 'LANG="en_US.UTF-8"' | tee -a /etc/default/locale && dpkg-reconfigure locales && update-locale LANG=en_US.UTF-8

USER build
WORKDIR /home/build

RUN echo " \
    alias q='/home/build/yocto/sources/eticos-linux-sdk/test-scripts/runqemu.py'\n\
    alias b='bitbake ticos-image'\n\
    alias cc='rm -rf /home/build/yocto/build/conf'\n\
    alias ct='rm -rf /home/build/yocto/build/tmp'\n\
    alias build-ticos-cli-local='pip3 install /home/build/ticos-cli-dist/ticos-cli-*.tar.gz'\n\
    alias upload-yocto-symbols='ticos --org \$TICOS_E2E_ORGANIZATION_SLUG --org-token \$TICOS_E2E_ORG_TOKEN --project \$TICOS_E2E_PROJECT_SLUG upload-yocto-symbols --image tmp/deploy/images/qemuarm64/base-image-qemuarm64.tar.bz2 --dbg-image tmp/deploy/images/qemuarm64/base-image-qemuarm64-dbg.tar.bz2'\n\
    alias upload-ota-payload='ticos --org \$TICOS_E2E_ORGANIZATION_SLUG --org-token \$TICOS_E2E_ORG_TOKEN --project \$TICOS_E2E_PROJECT_SLUG upload-ota-payload --hardware-version \${TICOS_HARDWARE_VERSION} --software-type \${TICOS_SOFTWARE_TYPE} --software-version 0.0.2 tmp/deploy/images/qemuarm64/swupdate-image-qemuarm64.swu' \n\
    " >> /home/build/.bashrc

# Install python testing dependencies
COPY requirements.txt /home/
RUN echo 'PATH=$HOME/.local/bin:$PATH' >> $HOME/.bashrc
RUN pip3 install --no-warn-script-location -r /home/requirements.txt

# Setup volume mounts
RUN mkdir -p /home/build/yocto/build
VOLUME /home/build/yocto/build
RUN mkdir -p /home/build/yocto/sources
VOLUME /home/build/yocto/sources
VOLUME /home/build/ticos-cli-dist

EXPOSE 5900

COPY entrypoint.sh /
COPY test.sh /
ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
