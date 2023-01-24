cmake_minimum_required(VERSION 3.13)
project(pr3)

set(CMAKE_CXX_STANDARD 14)

include_directories(cache)
include_directories(server)

add_executable(pr3
        cache/cache-student.h
        cache/gfserver.h
        cache/handle_with_cache.c
        cache/locals.txt
        cache/Makefile
        cache/shm_channel.c
        cache/shm_channel.h
        cache/simplecache.c
        cache/simplecache.h
        cache/simplecached.c
        cache/steque.c
        cache/steque.h
        cache/webproxy.c
        docs/part1.png
        docs/part2.png
        docs/PR3Settings05.png
        server/gfserver.h
        server/handle_with_curl.c
        server/handle_with_file.c
        server/locals.txt
        server/Makefile
        server/proxy-student.h
        server/steque.c
        server/steque.h
        server/webproxy.c
        server/workload.txt
        LICENSE
        readme-student.md
        Readme.md
        submit.py)
