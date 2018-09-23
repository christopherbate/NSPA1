#!/bin/bash
cp ./client_bin/test0.txt ./server_bin/test0.txt
rm ./server_bin/big.txt
rm ./client_bin/test0.txt
rm ./client_bin/test1.txt
make clean
make
cd server_bin
./server 8080 &
echo $! > ./server.pid
cd ../client_bin
./client localhost 8080 < test_cmd.txt
cd ..
echo `md5sum ./client_bin/big.txt`
echo `md5sum ./server_bin/big.txt`
echo `md5sum ./client_bin/test1.txt`
echo `md5sum ./server_bin/test1.txt`
kill -9 `cat ./server_bin/server.pid`
cp ./client_bin/test0.txt ./server_bin/test0.txt
rm ./server_bin/big.txt
rm ./client_bin/test0.txt
rm ./client_bin/test1.txt
rm ./server_bin/server.pid
