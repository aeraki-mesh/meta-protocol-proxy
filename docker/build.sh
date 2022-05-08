#!/bin/bash

BASEDIR=$(dirname "$0")
IMAGE_NAME="$1"

docker buildx create --use
docker buildx build -f ${BASEDIR}/build.Dockerfile -t ${IMAGE_NAME} --platform=linux/arm64,linux/amd64 . --push