#!/bin/bash

BASEDIR=$(dirname "$0")

trap 'onCtrlC' INT
function onCtrlC () {
    echo
    echo "cleaning up ..."

    docker stop brpc-echo-client brpc-echo-server  
    docker rm brpc-echo-client brpc-echo-server

    kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
    docker stop brpc-demo-envoy   
    docker rm brpc-demo-envoy 
}

if [ "$1" == "docker-build" ]
then
    echo "docker build mode"
    docker cp meta-protocol-brpc-build:/meta-protocol-proxy/bazel-bin/envoy $BASEDIR
    docker build -t brpc-demo-envoy -f $BASEDIR/envoy.Dockerfile .
    docker run -d --network host --name brpc-demo-envoy brpc-demo-envoy 
else
    echo "local build mode"
    $BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l debug&
fi

docker run -d -p 20001:20001 --name brpc-echo-server smwyzi/brpc-demo:2022-0508-0 \
    bash -c '/usr/local/bin/echo_server --port=20001'

docker run -d --network host --name brpc-echo-client smwyzi/brpc-demo:2022-0508-0 \
    bash -c 'echo "127.0.0.1 brpc-echo-server" >> /etc/hosts && /usr/local/bin/echo_client --server brpc-echo-server:20000'

docker logs -f brpc-echo-client
