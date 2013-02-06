#!/bin/sh

rm -f test/test-*

./client tcp 127.0.0.1 12345 test/test-1 &
./client tcp 127.0.0.1 12345 test/test-2 &
./client tcp 127.0.0.1 12345 test/test-3 &
./client tcp 127.0.0.1 12345 test/test-4 &
./client tcp 127.0.0.1 12345 test/test-5 &
