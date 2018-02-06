#!/bin/sh

#
# $Id: mpi.sh,v 1.3 2011/04/26 09:59:31 simakov Exp $
#
# Build MPI-enabled EPSILON version (MPICH)
# Please read README.mpich before!
#
# Hint: to simplify SSH keys management for the test, just
# put your public key into yours authorized_keys
#

SCRIPT_DIR=$(dirname $0)
source "$SCRIPT_DIR/common.inc"

BUILD_TAG='mpi'

check_EPSILON_TEST_BUILD_ROOT_environment_variable
BUILD_ROOT="$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG"

echo "Removing previous build: '$BUILD_ROOT'..."
rm -frv "$BUILD_ROOT"

echo "Building '$BUILD_TAG' EPSILON version..."
pushd "$EPSILON_SOURCE_ROOT"
trap 'popd' EXIT
make distclean
./configure --prefix "$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG" \
            --enable-mpi CC=mpicc                          \
            && make && make install

check_ldconfig $BUILD_TAG
