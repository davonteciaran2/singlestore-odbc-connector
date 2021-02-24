#!/bin/bash

set -eo pipefail

echo 127.0.0.1 singlestore.test.com | sudo tee -a /etc/hosts
export PROJ_PATH=`pwd`
mkdir tmp
.circleci/gen-ssl.sh singlestore.test.com tmp
export SSLCERT=$PROJ_PATH/tmp

# list ssl certificates
ls -lrt ${SSLCERT}

DEBIAN_FRONTEND=noninteractive sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get install --allow-unauthenticated -y --force-yes -m unixodbc-dev odbcinst1debian2 libodbc1 

## build odbc connector
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL
cmake --build . --config ${BUILD_TYPE}