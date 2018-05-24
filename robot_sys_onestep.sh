#!/bin/bash

# compile iec-runtime
echo "编译iec-runtime中"
cd ./iec-runtime/kernel/
make
cd ../../
echo "iec-runtime编译完成"
