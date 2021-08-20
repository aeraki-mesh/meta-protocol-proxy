#!/bin/bash
chmod +x bazel/get_workspace_status
export CC=clang-10
export CXX=clang++-10

export USE_BAZEL_VERSION=$(cat .bazelversion)

bazel version


# bazel clean --expunge

buildFlags=" -s --sandbox_debug --verbose_explanations --explain=build.log --verbose_failures "
# gdb opt fastbuild
if [ ! -z $1 ];then
    buildFlags+=" -c $1" //dgb, opt, fastbuild
else
    buildFlags=$buildFlags" -c fastbuild "
fi

target="envoy"

if [ `uname` = "Darwin" ];then
    bazel build ${buildFlags} //:$target --host_force_python=PY3
else
   bazel build ${buildFlags} //:$target
   #bazel build ${buildFlags}  //src/meta_protocol_proxy/filters/router/rds:string_accessor_lib
fi
