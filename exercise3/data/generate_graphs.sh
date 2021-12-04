#!/usr/bin/env bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${PROJECT_ROOT_DIR}/build"
DATA_DIR="${PROJECT_ROOT_DIR}/data"
GRAPH_CONSTRUCTION="${BUILD_DIR}/graph_construction"

cd "${DATA_DIR}" || exit 1

[ -f "${PROJECT_ROOT_DIR}/data.zip" ] && [ ! -f test_graph1.graph ] && unzip "${PROJECT_ROOT_DIR}/data.zip" -d PROJECT_ROOT_DIR

[ ! -x "${GRAPH_CONSTRUCTION}" ] && echo "${GRAPH_CONSTRUCTION} does not exist" && exit 1

# shellcheck disable=SC2046
[ ! -f 10x-1e5-2.graph ] && echo "generating 10x-1e5-2.graph" && ${GRAPH_CONSTRUCTION} 10 $(printf '100000 2 %.0s' {1..10}) >> 10x-1e5-2.graph
# shellcheck disable=SC2046
[ ! -f 10x-1e6-2.graph ] && echo "generating 10x-1e6-2.graph" && ${GRAPH_CONSTRUCTION} 10 $(printf '1000000 2 %.0s' {1..10}) >> 10x-1e6-2.graph
# shellcheck disable=SC2046
[ ! -f 10x-1e7-2.graph ] && echo "generating 10x-1e7-2.graph" && ${GRAPH_CONSTRUCTION} 10 $(printf '10000000 2 %.0s' {1..10}) >> 10x-1e7-2.graph
# shellcheck disable=SC2046
[ ! -f 1e3x-1e4-2.graph ] && echo "generating 1e3x-1e4-2.graph" && ${GRAPH_CONSTRUCTION} 1000 $(printf '10000 2 %.0s' {1..1000}) >> 1e3x-1e4-2.graph
# shellcheck disable=SC2046
[ ! -f 1e4x-1e4-2.graph ] && echo "generating 1e4x-1e4-2.graph" && ${GRAPH_CONSTRUCTION} 10000 $(printf '10000 2 %.0s' {1..10000}) >> 1e4x-1e4-2.graph
