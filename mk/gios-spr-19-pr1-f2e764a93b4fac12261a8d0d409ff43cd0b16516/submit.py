#!/usr/bin/env python
import os
import time
import argparse
import hashlib
import subprocess
import glob
from nelson.gtomscs import submit
# see https://github.com/udacity/nelson for nelson information (OMSCS specific)

DEVNULL = open("/dev/null", "wb")
cksum_file_name = "cksum.txt"

#Deletes cksum_file_name text file
def cleanup_cksum():
    try:
        os.remove(cksum_file_name)
    except OSError:
        pass

#Deletes zip file added by submit process
def cleanup_student_zip():
    try:
        os.remove('student.zip')
    except OSError:
        pass

#Hashes provided file blocks
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
    readme_candidates = ['readme-student.md', 'readme-student.pdf', 'student-readme.md', 'student-readme.pdf', 'submit.py']
    for candidate in readme_candidates:
        if os.path.isfile(candidate):
            readme_list.append(candidate)
    if len(readme_list) is 0:
        raise Exception("There is no valid student readme file to submit")
    return readme_list

def main():
    parser = argparse.ArgumentParser(description='Submits code to the Udacity site.')
    parser.add_argument('quiz', choices=['echo', 'transfer', 'gfclient', 'gfserver', 'gfclient_mt', 'gfserver_mt', 'readme'])

    args = parser.parse_args()

    path_map = {'echo': 'echo',
                'transfer': 'transfer',
                'gfclient': 'gflib',
                'gfserver': 'gflib',
                'gfclient_mt': 'mtgf',
                'gfserver_mt': 'mtgf',
                'readme': '.'}

    quiz_map = {'echo': 'pr1_echo_client_server',
                'transfer': 'pr1_transfer',
                'gfclient': 'pr1_gfclient',
                'gfserver': 'pr1_gfserver',
                'gfclient_mt': 'pr1_gfclient_mt',
                'gfserver_mt': 'pr1_gfserver_mt',
                'readme': 'pr1_readme'}

    files_map = {'pr1_echo_client_server': ['echoclient.c', 'echoserver.c'],
                 'pr1_transfer': ['transferclient.c', 'transferserver.c'],
                 'pr1_gfclient': ['gfclient.c', 'gfclient-student.h', 'gf-student.h'],
                 'pr1_gfserver': ['gfserver.c', 'gfserver-student.h', 'gf-student.h'],
                 'pr1_gfclient_mt': ['gfclient_download.c', 'gfclient-student.h', 'gf-student.h'],
                 'pr1_gfserver_mt': ['gfserver_main.c', 'handler.c', 'gfserver-student.h', 'gf-student.h'],
                 'pr1_readme': compute_readme_list()}

    quiz = quiz_map[args.quiz]

    os.chdir(path_map[args.quiz])
    
    #Create cksum file
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
