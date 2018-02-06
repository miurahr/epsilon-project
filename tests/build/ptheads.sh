#!/bin/sh

#
# $Id: ptheads.sh,v 1.2 2011/04/26 09:50:24 simakov Exp $
#
# Build multi-threaded EPSILON version (POSIX threads)
#

SCRIPT_DIR=$(dirname $0)
source "$SCRIPT_DIR/common.inc"

BUILD_TAG='pthreads'

check_EPSILON_TEST_BUILD_ROOT_environment_variable
BUILD_ROOT="$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG"

echo "Removing previous build: '$BUILD_ROOT'..."
rm -frv "$BUILD_ROOT"

echo "Building '$BUILD_TAG' EPSILON version..."
pushd "$EPSILON_SOURCE_ROOT"
trap 'popd' EXIT
make distclean
./configure --prefix "$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG" \
            --enable-pthreads                              \
            --with-def-threads=4                           \
            --with-max-threads=256                         \
            && make && make install

check_ldconfig $BUILD_TAG
