#!/bin/bash

if [ "x$1" = "x" -o "x$1" = "x-h" -o "x$1" = "x--help" ]; then
    printf <<EOF
Usage: $0 unitname
  e.g.: '$0 vp_stream' would make the following files:
    vp_stream.h
    lib/vp_stream_lib.h
    sys/vp_stream_sys.h
    core/vp_stream_core.h

EOF
    exit;
fi

_UNIT=`tr '[:lower:]' '[:upper:]' <<<"$1"`
_UNITLC=`tr '[:upper:]' '[:lower:]' <<<"$1"`

function make_header () {
    local skel=$1
    local target=$2

    if [ -e "$target" ]; then
        printf "WARNING: '$target' already exists!  Using '${target}_' instead!\n";
        make_header "$skel" "${target}_"
        return;
    fi

    printf "Making '$target' from skeleton file '$skel'...\n";

    sed -e "s,\\\${UNIT},${_UNIT},g" -e "s,\\\${unit},${_UNITLC},g" <$skel > $target

    return;
}

if ! pushd include >/dev/null; then
    echo "ERROR: Unable to chdir to include directory";
    exit 1;
fi

make_header _header_skel.h "${_UNITLC}.h"
make_header lib/_lib_header_skel.h "lib/${_UNITLC}_lib.h"
make_header sys/_sys_header_skel.h "sys/${_UNITLC}_sys.h"
make_header core/_core_header_skel.h "core/${_UNITLC}_core.h"

popd > /dev/null;

