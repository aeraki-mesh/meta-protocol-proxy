BASEDIR=$(dirname "$0")
docker kill consumer provider server client mirror-provider jaeger
docker rm consumer provider server client mirror-provider jaeger
docker run -d -p 20881:20880 --name provider ghcr.io/aeraki-mesh/dubbo-sample-provider
docker run -d -p 20882:20880 --name mirror-provider ghcr.io/aeraki-mesh/dubbo-sample-provider
sudo docker run -d --name jaeger --env COLLECTOR_ZIPKIN_HOST_PORT=":9411" -p 14268:14268 -p 16686:16686 -p 9411:9411 jaegertracing/all-in-one:1.22
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../../bazel-bin/envoy -c $BASEDIR/test.yaml -l info&
docker run --network host --name consumer --env mode=demo ghcr.io/aeraki-mesh/dubbo-sample-provider

