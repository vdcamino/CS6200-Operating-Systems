import time
import os
import sys
import argparse
import json
import datetime
import hashlib
import glob
from nelson.gtomscs import submit

cksum_file_name = "spr19-cksum.txt"

def cleanup_cksum():
  try:
      os.remove(cksum_file_name)
  except OSError:
      pass

def cleanup_student_zip():
  try:
    os.remove('student.zip')
  except OSError:
    pass

# from https://stackoverflow.com/questions/3431825/generating-an-md5-checksum-of-a-file
def hash_bytestr_iter(bytesiter, hasher, ashexstr=False):
    for block in bytesiter:
        hasher.update(block)
    return (hasher.hexdigest() if ashexstr else hasher.digest())

def file_as_blockiter(afile, blocksize=65536):
  with afile:
      block = afile.read(blocksize)
      while len(block) > 0:
          yield block
          block = afile.read(blocksize)

def compute_cksum(file_list):
  cksum = {}
  for file in file_list:
    if os.path.isdir(file): continue # skip directories
    try:
      stat = os.stat(file)
    except OSError:
      continue
    if (stat.st_size > 1024 * 1024): continue
    cksum[file] = hash_bytestr_iter(file_as_blockiter(open(file, 'rb')), hashlib.sha256(), True)
  return cksum

def get_submit_cksum(quiz=""):
    file_list = glob.glob("*")
    if quiz != "": file_list += glob.glob("../*")
    cksum = compute_cksum(file_list)
    with open(cksum_file_name, 'w+') as f:
        for key, value in cksum.items():
            f.write('%64.64s  %s\n' % (key, value))

def compute_readme_list():
    readme_list = []
    readme_candidates = ['readme-student.md', 'readme-student.pdf']
    for candidate in readme_candidates:
        if os.path.isfile(candidate):
            readme_list.append(candidate)
    if len(readme_list) is 0:
        raise Exception("There is no valid student readme file to submit")
    return readme_list

def main():
  parser = argparse.ArgumentParser(description='Submits code to the Udacity site.')
  parser.add_argument('quiz', choices = ['proxy_server', 'proxy_cache', 'readme'])
  
  args = parser.parse_args()

  path_map = { 'proxy_server': 'server', 
               'proxy_cache': 'cache', 
               'readme': '.'}

  quiz_map = { 'proxy_server': 'pr3_proxy_server', 
               'proxy_cache': 'pr3_proxy_cache', 
               'readme' : 'pr3_readme'}

  files_map = { 'pr3_proxy_server': ['handle_with_curl.c', 'webproxy.c', 'proxy-student.h'],
                'pr3_proxy_cache':  ['handle_with_cache.c', 'shm_channel.h',
                                     'webproxy.c', 'shm_channel.c', 'simplecached.c',
                                     'cache-student.h'],
            		'pr3_readme' : compute_readme_list()}

  quiz = quiz_map[args.quiz]
  cleanup_cksum()
  os.chdir(path_map[args.quiz])
  try:
    cleanup_cksum()
    get_submit_cksum(quiz)
  except:
    pass # ignore errors
  if (os.path.exists(cksum_file_name)): files_map[quiz].append(cksum_file_name)    

  submit('cs6200', quiz, files_map[quiz])
  cleanup_cksum()
  cleanup_student_zip()

if __name__ == '__main__':
  main()
