#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/.env
act -p=false --container-architecture linux/amd64 -P ubuntu-latest=termux-build-env:latest -j build -W .github/workflows/debug_build.yml -s GITHUB_TOKEN=$GITHUB_TOKEN --artifact-server-path ./build/artifacts
