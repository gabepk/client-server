# Client-Server

Simple client-server algorithm in C.

Client send request to a web page and write answer to a local file. The flag _-s_ is used when the user wants to overwrite the file.

Server listen to a port for incoming connections. The user can select the directory of server's origin.

## Client

$ gcc -Wall client.c -o client

$ ./client _URL_ _FileName_ [_-s_]

## Server

$ gcc -Wall server.c -o server

$ ./server _PORT_ _Directory_
