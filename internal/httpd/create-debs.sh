#!/bin/bash

# Copyright Splunk Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euxo pipefail

SCRIPT_DIR="$( cd "$( dirname ${BASH_SOURCE[0]} )" && pwd )"

INP_REPO=https://github.com/open-telemetry/opentelemetry-cpp-contrib
# this is nasty - fix me
INP_HTTPD_VER=v0.1.0
INP_FILENAME=mod-otel.so.zip

OUT_TAG=httpd/v0.1.0

create_one() {
  DISTRO_PREFIX=$1
  INP_ZIP="${INP_REPO}/releases/download/httpd%2F${INP_HTTPD_VER}/${DISTRO_PREFIX}_${INP_FILENAME}"
  curl ${INP_ZIP} -o mod_otel.so
  ${SCRIPT_DIR}/packaging/create-deb.sh
}

create_one "ubuntu-18.04"
