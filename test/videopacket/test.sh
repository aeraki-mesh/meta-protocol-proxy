#!/bin/bash
BASEDIR=$("pwd")
$BASEDIR/../../videopacket-bin/envoy -c $BASEDIR/test.yaml -l debug &