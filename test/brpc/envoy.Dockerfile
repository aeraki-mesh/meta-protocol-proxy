FROM ubuntu:18.04

RUN apt update -y \
    && apt install -y curl

ADD . /usr/local/bin

CMD /usr/local/bin/envoy -c /usr/local/bin/test.yaml -l debug