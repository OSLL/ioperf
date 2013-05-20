ampivovarov@gmail.com

Simple ramdisk block device driver
==================================

References:
-----------
* http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/ 
* LDD's "sbull" bdev  
* /drivers/block/brd.c - kernel ramdisk bdev

Modes:
------
* RM_SIMPLE - using kernel IO_Scheduler 
* RM_NOQUEUE - without scheduler
* RM_FULL - scheduler implementation inside bdev/hardware - not implemented

Usage:
------
    $ modprobe sbd
    $ mkfs /dev/sbd0
    $ mount /dev/sbd0 /mnt

params:
-------
    major_num - device major number
    logical_block_size - block size, bytes
    nsectors = number of sectors (device size == logical_block_size * nsectors)
