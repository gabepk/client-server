English/[Português](https://github.com/gabepk/client-server/blob/master/README.pt.md)

# Client-Server

Simple web client and web server algorithm in C.

## Client

Client send request to a **url** and write the payload of the response to a local file called **filename**. <br />
The flag **-s** is used if the user wants to overwrite this file.

> $ gcc -Wall client.c -o client <br />
> $ ./client [url] [filename] [-s]

## Server

Server listen to a **port** for incoming connections on the chosen **directory**).

> $ gcc -Wall server.c -o server  <br />
> $ ./server [port] [directory]
