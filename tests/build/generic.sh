#!/bin/sh

#
# $Id: generic.sh,v 1.1 2011/04/25 13:49:09 simakov Exp $
#
# Build generic EPSILON version
#

SCRIPT_DIR=$(dirname $0)
source "$SCRIPT_DIR/common.inc"

BUILD_TAG='generic'

check_EPSILON_TEST_BUILD_ROOT_environment_variable
BUILD_ROOT="$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG"

echo "Removing previous build: '$BUILD_ROOT'..."
rm -frv "$BUILD_ROOT"

echo "Building '$BUILD_TAG' EPSILON version..."
pushd "$EPSILON_SOURCE_ROOT"
trap 'popd' EXIT
make distclean
./configure --prefix "$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG" && make && make install

check_ldconfig $BUILD_TAG
