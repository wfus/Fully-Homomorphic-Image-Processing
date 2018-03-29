#!/bin/sh

git submodule update --init --recursive


# Installing SEAL
cd SEAL/SEAL
./configure "CXX=g++-7"
make
cd ../..






