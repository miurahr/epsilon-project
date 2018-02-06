#!/bin/sh

#
# $Id: cluster.sh,v 1.2 2011/04/26 09:59:31 simakov Exp $
#
# Build cluster EPSILON version (self-made TCP based solution)
# Please read README.cluster before!
#

SCRIPT_DIR=$(dirname $0)
source "$SCRIPT_DIR/common.inc"

BUILD_TAG='cluster'

check_EPSILON_TEST_BUILD_ROOT_environment_variable
BUILD_ROOT="$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG"

echo "Removing previous build: '$BUILD_ROOT'..."
rm -frv "$BUILD_ROOT"

echo "Stopping running EPSILON nodes..."
killall epsilon

echo "Building '$BUILD_TAG' EPSILON version..."
pushd "$EPSILON_SOURCE_ROOT"
trap 'popd' EXIT
make distclean
./configure --prefix "$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG" \
            --enable-pthreads                              \
            --enable-cluster                               \
            && make && make install

check_ldconfig $BUILD_TAG

for port in `cat "./tests/build/epsilon.nodes" | perl -ne 'print "$1\n" if /\:(\d+)\^/'`; do
    echo "Starting EPSILON node on port $port..."
    "$EPSILON_TEST_BUILD_ROOT/$BUILD_TAG/bin/epsilon" --start-node --port $port
done

if ! ps aux | grep [e]psilon; then
    echo "WARNING: EPSILON node hasn't been started. Please check syslog messages (daemon facility)"
fi
