#!/bin/bash
BASEDIR=$("pwd")
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l debug &

cd $BASEDIR/helloworld/server
go build -o trpc-server
$BASEDIR/helloworld/server/trpc-server&

cd $BASEDIR/helloworld/client
go build -o trpc-client
$BASEDIR/helloworld/client/trpc-client -addr=ip://127.0.0.1:28000
