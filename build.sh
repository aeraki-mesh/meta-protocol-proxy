#!/bin/bash
chmod +x bazel/get_workspace_status
export CC=clang-10
export CXX=clang++-10

export USE_BAZEL_VERSION=$(cat .bazelversion)

bazel version


# bazel clean --expunge

buildFlags="-s --sandbox_debug "
# gdb opt fastbuild
if [ ! -z $1 ];then
    buildFlags+=" -c $1"
else
    buildFlags=$buildFlags" -c fastbuild"
fi

target="envoy"

if [ `uname` = "Darwin" ];then
    bazel build ${buildFlags} //:$target --host_force_python=PY3
else
    bazel build -c dbg ${buildFlags} //:$target
fi
