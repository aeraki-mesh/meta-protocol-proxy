#!/bin/bash

BASEDIR=$(dirname "$0")
docker kill consumer provider server client
docker rm consumer provider server client
docker run -d --network host --name client --env helloServer=localhost --env mode=demo aeraki/thrift-sample-client
docker run -d -p 9091:9090 --name server aeraki/thrift-sample-server
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test-full.yaml &
#$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test-default-bucket.yaml &
#$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test-only-descriptor.yaml &
#$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test-without-match.yaml &
docker logs -f client
