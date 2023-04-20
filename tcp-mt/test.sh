#!/usr/bin/env bash

cd "$(dirname "$0")" || exit 1
msg_dir=${1:-./messages}
host=${2:-0.0.0.0}
port=${3:-3333}

function free_port {
    lsof -i ":$1" | awk 'NR>1{print $2}' | xargs kill 2> /dev/null
}

free_port "$port" 
cd build || exit 1
./file-server-tcp "$port" &
cd ..
find "$msg_dir/" -type f -exec ./build/file-client-tcp "$host" "$port" {} \; || exit 1
sleep 1s
free_port "$port"
