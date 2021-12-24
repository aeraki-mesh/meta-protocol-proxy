#!/bin/bash
BASEDIR=$("pwd")
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l debug &