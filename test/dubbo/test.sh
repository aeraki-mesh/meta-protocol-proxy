BASEDIR=$(dirname "$0")
docker kill consumer provider server client mirror-provider
docker rm consumer provider server client mirror-provider
docker run -d --network host --name consumer --env mode=test aeraki/dubbo-sample-consumer
docker run -d -p 20881:20880 --name provider aeraki/dubbo-sample-provider
docker run -d -p 20882:20880 --name mirror-provider aeraki/dubbo-sample-provider
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l trace&
docker logs -f consumer
