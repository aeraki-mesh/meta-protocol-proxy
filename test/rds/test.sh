BASEDIR=$(dirname "$0")
docker kill consumer provider server client example-rds-server
docker rm consumer provider server client example-rds-server
docker run -d --network host --name consumer --env mode=demo aeraki/dubbo-sample-consumer
docker run -d -p 20881:20880 --name provider aeraki/dubbo-sample-provider
docker run -d -p 18000:18000 --name example-rds-server aeraki/example-rds-server
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml &
docker logs -f consumer
