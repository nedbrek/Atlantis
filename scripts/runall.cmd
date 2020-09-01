#!/bin/sh
find . -name 'turn.*' -printf 'cd %f; ../../neworigins_v2 run\0' | xargs -0 -P 6 -L 1 bash -c

