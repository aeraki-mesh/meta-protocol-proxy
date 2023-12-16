#!/bin/bash
BASEDIR=$(dirname "$0")

docker kill client server
docker rm client server
docker run  --network host -d --name server aeraki/trpc-server
docker run  --network host -d --env server_addr=127.0.0.1:28000 --env c=1 --env cmd=SayHello --name client aeraki/trpc-client
#docker run  --network host --env server_addr=127.0.0.1:28000 --env c=1 --env cmd=SayHelloClientStream --name client aeraki/trpc-client
#docker run  --network host --env server_addr=127.0.0.1:28000 --env c=1 --env cmd=SayHelloServerStream --name client aeraki/trpc-client
#docker run  --network host --env server_addr=127.0.0.1:28000 --env c=1 --env cmd=SayHelloBidirectionStream --name client aeraki/trpc-client
#docker logs -f server
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l debug
