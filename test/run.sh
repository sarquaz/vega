#!/bin/bash

shopt -s nullglob
for f in test/test_*.lua
do
    
then
    echo "running $f"
    ./vega "$f"
    
    if [ $? -ne 0 ]; then
        exit -1
    fi



done
 