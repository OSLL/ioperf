#!/usr/bin/env python
#coding=utf-8
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
      #print row    
      rm_temp = ["rm"] + glob.glob(row[1] + "*") 
      #print rm_temp
      if len(rm_temp) == 2: subprocess.call(rm_temp)

