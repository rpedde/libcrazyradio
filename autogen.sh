#!/bin/bash

for req in libtoolize autoreconf; do
    if ! hash ${req}; then
        echo "autogen.sh: error: cannot find program: ${req}"
        exit 1
    fi
done

for d in $(dirname $(readlink -f $0))/config $(dirname $(readlink -f $0))/m4; do
    if ! mkdir -p $(dirname $(readlink -f $0))/config; then
        echo "autogen.sh: error: cannot create directory: $d "
        exit 1
    fi
done

if ! autoreconf --install --force --verbose -I config; then
    echo "autogen.sh: error: cannot execute autoreconf"
    exit 1
fi
