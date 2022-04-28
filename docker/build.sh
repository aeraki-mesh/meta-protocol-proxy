#!/bin/bash

docker buildx create --use
docker buildx build -f ./build.Dockerfile -t smwyzi/meta-protocol-proxy-build:2022-0429-0 --platform=linux/arm64,linux/amd64 . --push