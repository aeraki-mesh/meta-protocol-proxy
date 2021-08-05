BASEDIR=$(dirname "$0")
docker kill consumer provider
docker rm consumer provider
docker run -d --network host --name consumer aeraki/dubbo-sample-consumer
docker run -d -p 20881:20880 --name provider aeraki/dubbo-sample-provider
kill `ps -ef | awk '/bazel-bin\/envoy/{print $2}'`
$BASEDIR/../bazel-bin/envoy -c $BASEDIR/test.yaml &
while true
do
  sleep 3
  curl 127.0.0.1:9009/hello
  echo " "
done
