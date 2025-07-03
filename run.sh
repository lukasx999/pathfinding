#!/bin/sh
set -euxo pipefail

# osmosis --read-pbf austria-latest.osm.pbf --node-key-value keyValueList="building.*" --write-xml map.osm
# osmosis --read-pbf austria-latest.osm.pbf --tf accept-ways amenity=college --used-node --write-xml map.osm

c++ main.cc ./tinyxml2.cpp -o pathfinding -Wall -Wextra -std=c++23 -pedantic -O3 -ggdb -lraylib # -fsanitize=address,undefined

time ./pathfinding
