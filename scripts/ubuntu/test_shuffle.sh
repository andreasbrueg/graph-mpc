#!/bin/bash

cd ./build/tests/test_shuffle && (./test_shuffle -p 0 --localhost & ./test_shuffle -p 1 --localhost & ./test_shuffle -p 2 --localhost)