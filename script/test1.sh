#!/bin/bash

./supermercato &
p=$!
sleep 25
kill -3 $p
wait $p
./script/analisi.sh
