#!/bin/bash

afl-fuzz -i ./afl-seeds -o ./afl-output -x afl-dict.txt -D -- ./ars2000-afl
