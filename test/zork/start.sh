docker stop envoy
docker rm envoy
docker run -idt --name envoy -v "/home/gongenzhao/project/metaProtocol/meta-protocol-proxy/test/zork/test.yaml:/etc/envoy/envoy.yaml" -v "/home/gongenzhao/project/metaProtocol/meta-protocol-proxy/test/zork/logs:/tmp" -p 20000:20000 -p 9333:15000 envoy-multi-dispatch:v1
