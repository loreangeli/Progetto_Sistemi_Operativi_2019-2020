#!/bin/bash

./supermercato &
p=$!
sleep 25
kill -1 $p
wait $p
./script/analisi.sh
