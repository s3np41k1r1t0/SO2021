#!/bin/bash

#i know this is bad but its only a temporary thing lol

cd client
make
mv tecnicofs ../tecnicofs_client
cd ../server
make
mv tecnicofs ../tecnicofs_server
