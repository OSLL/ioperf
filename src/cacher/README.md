author: Vadim Lomshakov (vadim.lomshakov@gmail.com)

Cacher
======

Info:
------
Utility functions for io perfomance test: 
* enumeration page into cache
* force pages from cache
* add range pages into cache

Creates folder '/sys/ioperf_cache_utility/' contains folders basic function: force_cache, add_page_range, ...
Inside these folders contains the settings files and startup file("start").

Build module:
-------------------
	$ make -C pathname/to/linux/kernel/build M=`pwd` modules
	$ sudo insmod utilitiesmain.ko

Usage:
----------
	$ add_page_range.sh pathname begin_range end_range
	$ enumerate_cached_pages.sh pathname
	$ force_cache.sh pathname

params:
-----------
	pathname is absolute
	bagin_/end_range is positive integer number
        