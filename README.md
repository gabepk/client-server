# Client-Server

Simple client-server algorithm in C.

Client send request to a web page and write answer to a local file.

Server listen to a port for incoming connections. The user can select the directory of server's origin.

## Client

$ gcc -Wall client.c -o client

$ ./client <URL> <File name> <-s to overwrite file>

## Server

$ gcc -Wall server.c -o server

$ ./server <PORT> <Directory>
