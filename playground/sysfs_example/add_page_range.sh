#!/bin/bash

if [[ "$2" =~ ^[0-9]+$ && "$3" =~ ^[0-9]+$ && -f "$1" ]] ; then
  echo "$2 $3" > /sys/ioperf_cache_utility/add_page_range/range
  echo "$1" > /sys/ioperf_cache_utility/add_page_range/pathname
  echo "1" > /sys/ioperf_cache_utility/add_page_range/start
else
  exec >&2; echo -e "error: wrong args \nusage:add_page_range.sh pathname begin_range end_range"; exit 1
fi

