#!/bin/sh -e

# NB: YOCTO_RELEASE is searched and replaced by linux_sdk_release.py during archival.
# Treat is as generated code, or update linux_sdk_release.py accordingly.
YOCTO_RELEASE="kirkstone"

command=""

TICOS_YOCTO_BUILD_MOUNT_PREFIX=${TICOS_YOCTO_BUILD_MOUNT_PREFIX:-"/tmp/yocto-build-"}
buildmount="--mount type=volume,source=yocto-build-${YOCTO_RELEASE},target=/home/build/yocto/build"

while getopts "bv:c:e:r:tv:" options; do
    case "${options}" in
    b)
        docker build --tag yocto .
        ;;
    c)
        command="${OPTARG}"
        ;;
    e)
        entrypoint="--entrypoint ${OPTARG}"
        ;;
    r)
        YOCTO_RELEASE="${OPTARG}"
        ;;
    t)
        # Use a bind mount at ${TICOS_YOCTO_BUILD_MOUNT_PREFIX}${YOCTO_RELEASE} for the build artifacts
        # for easy inspection of output. Example usage:
        #
        # $ TICOS_YOCTO_BUILD_MOUNT_PREFIX=${HOME}/yocto ./run.sh -bt
        #
        # The build artifacts will be placed at ${HOME}/yocto-${YOCTO_RELEASE}`
        mkdir -p "${TICOS_YOCTO_BUILD_MOUNT_PREFIX}${YOCTO_RELEASE}"
        buildmount="--mount type=bind,source=${TICOS_YOCTO_BUILD_MOUNT_PREFIX}${YOCTO_RELEASE},target=/home/build/yocto/build"
        ;;
    *) exit 1;;
    esac
done

metamount="--mount type=bind,source=${PWD}/..,target=/home/build/yocto/sources/observability-linux-sdk"
sourcesmount="--mount type=volume,source=yocto-sources-${YOCTO_RELEASE},target=/home/build/yocto/sources"

if [ -n "$TICOS_CLI_DIST_PATH" ];
  then ticosclimount="--mount type=bind,source=$(readlink -f $TICOS_CLI_DIST_PATH),target=/home/build/ticos-cli-dist"
  else ticosclimount=""
fi

# vars are overridden from the local environment, falling back to env.list
env_vars="
--env MACHINE
--env TICOS_BASE_URL
--env TICOS_PROJECT_KEY
--env TICOS_DEVICE_ID
--env TICOS_SOFTWARE_VERSION
--env TICOS_HARDWARE_VERSION
--env TICOS_SOFTWARE_TYPE
--env-file env.list
"

# vars for E2E test scripts
e2e_test_env_vars="
--env TICOS_E2E_API_BASE_URL
--env TICOS_E2E_ORGANIZATION_SLUG
--env TICOS_E2E_PROJECT_SLUG
--env TICOS_E2E_ORG_TOKEN
--env-file env-test-scripts.list
"

docker run \
    --interactive --rm --tty \
    --network="host" \
    --name ticos-linux-qemu \
    ${buildmount} \
    ${sourcesmount} \
    ${ticosclimount} \
    ${metamount} \
    ${env_vars} \
    ${e2e_test_env_vars} \
    --env YOCTO_RELEASE="${YOCTO_RELEASE}" \
    ${entrypoint} \
    yocto \
    ${command}
