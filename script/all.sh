#!/bin/bash

./supermercato &
p=$!
wait $p
clear 
./script/analisi.sh
