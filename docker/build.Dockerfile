FROM ubuntu:18.04

ARG CLANG_FILE_AMD64=clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
ARG CLANG_FILE_ARM64=clang+llvm-10.0.0-aarch64-linux-gnu.tar.xz
ARG CLANG_SAVE_FILE=clang+llvm-10.0.0-linux-gnu.tar.xz
ARG CLANG_SAVE_DIR=clang+llvm-10.0.0-linux-gnu

RUN apt update -y && \
    apt install -y wget git vim python \
        autoconf automake cmake curl libtool make ninja-build patch python3-pip unzip virtualenv libc++-10-dev && \
    wget -O /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-$([ $(uname -m) = "aarch64" ] && echo "arm64" || echo "amd64") && \
    chmod +x /usr/local/bin/bazel && \
    mkdir -p /home/ubuntu && cd /home/ubuntu && \
    wget -O $CLANG_SAVE_FILE https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/$([ $(uname -m) = "aarch64" ] && echo $CLANG_FILE_ARM64 || echo $CLANG_FILE_AMD64) && \
    mkdir $CLANG_SAVE_DIR && \
    tar -xvf $CLANG_SAVE_FILE -C $CLANG_SAVE_DIR --strip-components 1 && \
    rm $CLANG_SAVE_FILE    

