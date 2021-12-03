#!/bin/bash

DATA_FILE=efficient-parallel-cpp-ex4.zip

cd ${0%/*}

if [[ -e $DATA_FILE ]]; then
    echo 'Data already downloaded; exiting'
    exit 0
fi

if wget https://algo2.iti.kit.edu/download/$DATA_FILE; then
    if [[ -x $(which unzip) ]]; then
        unzip $DATA_FILE
    else
        echo 'unzip not found, cannot extract data'
        exit 2
    fi
else
    echo 'Error downloading data'
    exit 1
fi


