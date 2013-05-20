#!/usr/bin/env python2
#coding=utf-8
# Copyright: vadim.lomshakov@gmail.com

import subprocess
import csv
import os
import glob

csv.register_dialect('fio',delimiter=';')
DIR_SET_TEST = "./set_test/"
tests = os.listdir(DIR_SET_TEST)

for cur_test in tests:
  p = subprocess.Popen("fio --minimal " + DIR_SET_TEST + cur_test, shell = True, stdout =  subprocess.PIPE)
  p.wait();
  output = reduce(lambda x,y:x+y ,[],p.stdout.readlines())  
  output = output[1:-1]
  reader = csv.reader(output, dialect='fio')
  for row in reader:
    if len(row):
      print "criterion: %s, bandwidth=%s(KB/sec)"%(row[1],row[5])  
      rm_temp = ["rm"] + glob.glob(row[1] + "*") 
      if len(rm_temp) == 2: subprocess.call(rm_temp)
