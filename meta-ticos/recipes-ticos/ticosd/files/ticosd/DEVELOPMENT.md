# Development

## Building outside Yocto

### Installing dependencies

#### On Debian

```sh
apt install \
  cpputest \
  libcurl4-openssl-dev \
  libjson-c-dev \
  libsystemd-dev \
  libubootenv-dev
```

### Building

```sh
BUILD_DIR=build

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake .. # Point it at the directory where DEVELOPMENT.md lives.
make
```

You may choose anything as your build directory, but currently we have some in
`.gitignore`:

- `/build` at the root of the `observability-linux-sdk-internal` repository, and
- `cmake-build-*/` anywhere in the `observability-linux-sdk-internal` repository—this
  is what CLion uses.

## Running tests

Do this after running a build, inside the build directory:

```sh
make test
```

### Inside Docker

A helper script called `/test.sh` is part of the Docker image that runs
`ticosd`'s CppUTest unit tests.

From within the container, run:

```console
/test.sh
```

Or from the host:

```console
./run.sh -b -e /test.sh
```

## IDE integration

### Using CLion to work on ticosd

[Install dependencies][#installing-dependencies] first.

- Add `-DPLUGIN_REBOOT=1` (and any other plugins you want to compile in) to the
  CMake arguments in Clion's Settings.
- If you are using a conda env, add
  `-DPKG_CONFIG_EXECUTABLE=<path/to/pkg-config>` to the CMake arguments, to make
  sure the correct `pkg-config` binary is used.
- Find meta-ticos/recipes-ticos/ticosd/files/ticosd/CMakeLists.txt
  in the Project.
- Right click it and select "Load Cmake Project".
- `ticosd` and various `test_...` targets are now available to build, run and
  debug from CLion!
