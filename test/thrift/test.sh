BASEDIR=$(dirname "$0")
docker kill consumer provider server client
docker rm consumer provider server client
docker run -d --network host --name client --env helloServer=localhost --env mode=demo aeraki/thrift-sample-client
docker run -d -p 9091:9090 --name server aeraki/thrift-sample-server
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l debug &
docker logs -f client
