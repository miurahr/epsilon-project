#!/bin/sh

#
# $Id: common.inc,v 1.2 2011/04/25 14:13:15 simakov Exp $
#
# Common snippets for EPSILON build scripts
#

SCRIPT_DIR=$(dirname $0)
EPSILON_SOURCE_ROOT="$SCRIPT_DIR/../.."

function check_EPSILON_TEST_BUILD_ROOT_environment_variable() {
    if [ -z "$EPSILON_TEST_BUILD_ROOT" ]; then
        echo "Please set EPSILON_TEST_BUILD_ROOT environment variable"
        exit 1
    fi

    if ! [ -d "$EPSILON_TEST_BUILD_ROOT" ]; then
        echo "No such directory: $EPSILON_TEST_BUILD_ROOT"
        exit 1
    fi
}

function check_ldconfig() {
    echo "Updating so libs cache using ldconfig..."
    sudo ldconfig

    if ! sudo ldconfig -p | fgrep "$EPSILON_TEST_BUILD_ROOT/$1/lib/libepsilon"; then
        echo "WARNING: please add '$EPSILON_TEST_BUILD_ROOT/$1/lib' to your /etc/ld.so.conf and run 'sudo ldconfig'"
    fi
}
