#!/bin/bash


if [[ -f "$1" ]] ; then
  echo "$1" > /sys/ioperf_cache_utility/force_cache/pathname
  echo "1" > /sys/ioperf_cache_utility/force_cache/start
else
  exec >&2; echo -e "error: wrong args \nusage:force_cache.sh pathname"; exit 1
fi


