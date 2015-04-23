#!/bin/bash
if [[ $# != 1 ]]; then
  echo >&2 "Usage: $0 user"
  exit 2
fi
set -x
user=$1
mkdir -p users
chown $user:$user users
chattr +a users
