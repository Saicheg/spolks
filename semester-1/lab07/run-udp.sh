#!/bin/sh

rm -f test/test-*

./client udp 127.0.0.1 12345 test/test-1 &
./client udp 127.0.0.1 12345 test/test-2 &
./client udp 127.0.0.1 12345 test/test-3 &
./client udp 127.0.0.1 12345 test/test-4 &
./client udp 127.0.0.1 12345 test/test-5 &
