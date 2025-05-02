#!/bin/bash

gnome-terminal -- bash -c "cd ./build/tests/test_shuffle && ./test_shuffle -p 0 --localhost"
gnome-terminal -- bash -c "cd ./build/tests/test_shuffle && ./test_shuffle -p 1 --localhost"
gnome-terminal -- bash -c "cd ./build/tests/test_shuffle && ./test_shuffle -p 2 --localhost"