/*$Id: $*/
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 80

FILE *fp;
char msg[1024];
char host[1024];

void connect_to_socket ()
{
  int sockfd, is_connected;
  struct sockaddr_in server_addr;
  struct hostent *server;


  /* Conexao com servidor */

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


  /* Envio e recebimento de mensagens */

  int bytes_sent, bytes_recv;
  int offset = 0;
  int i, j=0, total;
  char content_length[4];
  char buffer[1024];
 
  bytes_sent = write(sockfd, msg, strlen(msg));
  printf("Bytes sent: %d\n", bytes_sent);
  printf("Message sent: \n%s\n", msg);
  
  printf("Message received: \n");
  memset(buffer, 0, sizeof(buffer));
/*
  bytes_recv = read(sockfd, buffer, sizeof(buffer));
  if (bytes_recv > 0) {
    if ((i = strstr(buffer, "Content-Length: ")) != NULL) {
      while (buffer[i+16] != '\r') {
        content_length[j] = buffer[i+16];
	i++;j++;
      }
    }
  }*/
  
  

  do {
    bytes_recv = read(sockfd, buffer + offset, strlen(buffer));
    if (bytes_recv < 0) {
      printf("Erro ao ler mensagem do socket.\n");
    }
    else if (bytes_recv == 0) {
      printf("\n\nFim da mensagem recebida.\n");
      break;
    }
    
    offset += bytes_recv;
    fprintf(fp, "> %s\n\n", buffer);
  } while (bytes_recv < strlen(buffer));

  close(sockfd);
  return;
}

void set_msg (char *path) {
  char length[4];

  //strncpy(msg, "GET ", 5);
  //strncat(msg, path, strlen(path));
  //strncat(msg, " HTTP/1.0\r\n", 12);
  
  snprintf(msg, 16+strlen(path), "GET %s HTTP/1.0\r\n", path);
  
  //strncat(msg, "Content-Type: text/html\r\n", 26);
  //strncat(msg, "Content-Length: ", 17);
  //sprintf(lengthed "%d", strlen(msg) + 4);
  //strncat(msg, length, 4);
  //strncat(msg, "\r\n", 3);
  return;
}


int main (int argc, char *argv[]) {
  
  /* TODO Verificar entrada "./r 1 -s"*/
  if (argc < 3) { 
    printf("Informe <URL> <nome do arquivo> <-s para sobrescrever arquivo>\n");
    exit(1);
  }
  
  fp = fopen(argv[2], "r+");
  /*Arquivo ja existe, e flag de sobrescrita foi informada*/
  if (fp) {
    if (argv[3] && strncmp(argv[3], "-s", 2) != 0) {
      printf("Arquivo ja existe e nao pode ser sobrescrito.\n");
      printf("Para sobreescreve-lo, adicione nos parametros a flag -s.\n");
      exit(1);
    }
    /*pode sobreescrever*/
  }
  else {
    fp = fopen(argv[2], "a+");
  }
  
  /* Parser da URL usando Regex */
  /*
  regex_t reg;
  int is_regex_valid;
  char url_pattern[40];
  
  strncpy(url_pattern, "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)", 40);
  is_regex_valid = regcomp(&reg, url_pattern, REG_EXTENDED|REG_NOSUB); 
  
  if (is_regex_valid == 0) {
    if ((&reg != NULL) && (regexec(&reg, argv[1], (size_t) 0, (regmatch_t *) NULL, 0) == 0)) {
      printf("host ok\n");
     // regfree(&reg);
    }
  }
  */

  set_msg(argv[1]);
  strncat(host, argv[1], strlen(argv[1]));

  connect_to_socket();
  

  fclose(fp);
  return 0;
}
