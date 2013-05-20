#!/bin/bash


if [[ -f "$1" ]] ; then
  echo "$1" > /sys/ioperf_cache_utility/enumerate_cached_pages/pathname
  echo "1" > /sys/ioperf_cache_utility/enumerate_cached_pages/start
  cat /sys/ioperf_cache_utility/enumerate_cached_pages/log
else
  exec >&2; echo -e "error: wrong args \nusage:enumerate_cached_page.sh pathname"; exit 1
fi



