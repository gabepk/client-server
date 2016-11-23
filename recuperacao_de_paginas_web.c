/*$Id: $*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 80
#define BUFFER_SIZE 5
#define MAX_STRING_SIZE 1024

FILE *fp;
int sockfd;
char msg[MAX_STRING_SIZE];
char host[MAX_STRING_SIZE];
char buffer[BUFFER_SIZE];

void connect_to_socket ()
{
  struct sockaddr_in server_addr;
  struct hostent *server;
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if ( sockfd < 0)
  {
    printf("Erro: Nao foi possivel obter descritor do socket.\n");
    exit(1);
  }

  server = gethostbyname(host);
  if (!server) {
    printf("Erro: Host %s nao existe.\n", host);
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) 
      < 0) {
    printf("Erro: Nao foi possivel se conectar no Host %s, na porta %d.\n", 
           host, PORT);
  }
  
  return;
}

void send_message(int sockfd) {
  int bytes_sent;

  bytes_sent = send(sockfd, msg, strlen(msg), 0);
  if (bytes_sent < 0) {
    printf("Erro ao enviar mensagem para o socket.\n");
    exit(1);
  }
  printf("\n[Message sent] \t%s", msg);
  
  return;
}

void recv_message(int sockfd) {
  int bytes_recv;

  do {
    memset(buffer, 0, BUFFER_SIZE);
    bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_recv < 0) {
      printf("Erro ao receber mensagem do socket.\n");
      exit(1);
    }
    else if (bytes_recv == 0) {
      break;
    }
    fprintf(fp, "%s", buffer);
  } while (bytes_recv > 0);
  
  return;
}
  
int main (int argc, char *argv[]) {
  
  /* TODO Verificar entrada "./r 1 -s"*/
  if (argc < 3) { 
    printf("Informe <URL> <nome do arquivo> <-s para sobrescrever arquivo>\n");
    exit(1);
  }
  
  fp = fopen(argv[2], "r+");
  /* Arquivo ja existe */
  if (fp) {
    /* Se flag de sobrescrita nao foi informada, exit. */
    if (argv[3] && strncmp(argv[3], "-s", 2) != 0) {
      printf("Arquivo ja existe e nao pode ser sobrescrito.\n");
      printf("Para sobreescreve-lo, adicione nos parametros a flag -s.\n");
      exit(1);
    }
  }
  else {
    fp = fopen(argv[2], "a+");
  }
  
  strncat(host, "server.aker.com.br", 20);
  snprintf(msg, 58, "GET server.aker.com.br HTTP/1.0\r\n\r\n");
  //snprintf(msg, 18+strlen(path), "GET %s HTTP/1.0\r\n\r\n", path);

  connect_to_socket();
  send_message(sockfd);
  recv_message(sockfd);
  close(sockfd);  
  fclose(fp);
  return 0;
}
