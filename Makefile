## Copyright Aeraki Authors
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

SHELL := /bin/bash
CC := clang-10
CXX := clang++-10
PATH := /home/ubuntu/clang+llvm-10.0.0-linux-gnu/bin:$(PATH)

BAZEL_CONFIG = -s --sandbox_debug --verbose_failures --verbose_explanations --explain=build.log --host_force_python=PY3
BAZEL_CONFIG_DEV  = $(BAZEL_CONFIG) --config=libc++
BAZEL_CONFIG_REL  = $(BAZEL_CONFIG_DEV) --config=release
BAZEL_TARGETS = envoy

build:
	export PATH=$(PATH) CC=$(CC) CXX=$(CXX) && \
	bazel build $(BAZEL_CONFIG_DEV) $(BAZEL_TARGETS)

release:
	export PATH=$(PATH) CC=$(CC) CXX=$(CXX) && \
	bazel build $(BAZEL_CONFIG_REL) $(BAZEL_TARGETS)

# output files are in this location: bazel-bin/api/meta_protocol_proxy
api:
	bazel build //api/meta_protocol_proxy/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/admin/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/filters/router/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/filters/local_ratelimit/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/filters/global_ratelimit/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/filters/metadata_exchange/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/filters/stats/v1alpha:pkg_go_proto && \
	bazel build //api/meta_protocol_proxy/config/route/v1alpha:pkg_go_proto

clean:
	@bazel clean

.PHONY: build clean api
