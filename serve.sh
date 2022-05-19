#!/bin/bash

port=${1:-31337}

echo "connect with:
  nc <IP> $port
"

while true; do nc -v -l -p "$port" -e ./roll80s; done
