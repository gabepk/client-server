Português/[English](https://github.com/gabepk/client-server/blob/master/README.md)

# Cliente e Servidor

Algoritmo simples em C de um cliente e um servidor.

## Client

O cliente faz uma requisição para uma página na web (**url**) e escreve o payload da resposta em um arquivo (**filename**). <br />
A flag **-s** pode ser usada caso o usuário queira sobrescrever o arquivo.

> $ gcc -Wall client.c -o client <br />
> $ ./client [url] [filename] [-s]

## Server

O servidor escuta chegada de conexões na porta (**port**), e no diretório (**directory**) desejado. <br />

> $ gcc -Wall server.c -o server  <br />
> $ ./server [port] [directory]
