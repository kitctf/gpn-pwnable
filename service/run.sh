#!/bin/bash
if [[ $# != 4 ]]; then
    echo >&2 "Usage: $0 host port directory username"
    exit 2
fi
host=$1
port=$2
dir=$3
username=$4
echo "Host: $host"
echo "Port: $port"
echo "Directory: $dir"
echo "Username: $username"
socat tcp4-l:$port,bind=$host,fork,reuseaddr exec:"./adDOCtive $dir $username",stderr
