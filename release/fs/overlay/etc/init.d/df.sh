#!/bin/sh

while true
do
    df -Th > /disk/df.txt
    sleep 30
done
