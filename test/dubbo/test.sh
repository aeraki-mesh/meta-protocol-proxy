BASEDIR=$(dirname "$0")
docker kill consumer provider server client
docker rm consumer provider server client
docker run -d --network host --name consumer --env mode=demo aeraki/dubbo-sample-consumer
docker run -d -p 20881:20880 --name provider aeraki/dubbo-sample-provider
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml &
docker logs -f consumer
