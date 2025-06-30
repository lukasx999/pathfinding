#!/bin/sh
set -euxo pipefail

c++ -Wall -Wextra -std=c++23 -pedantic -Og -ggdb main.cc -lraylib
./a.out
