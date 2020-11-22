#!/bin/bash

#i know this is bad but its only a temporary thing lol

rm tecnicofs_server tecnicofs_client
cd client
make clean
mv tecnicofs ../tecnicofs_client
cd ../server
make clean
mv tecnicofs ../tecnicofs_server

