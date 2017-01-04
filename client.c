/*!
 * \file  client.c
 * \brief Recuperador de Paginas Web
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 164
#define HEADER_BUFFER_SIZE 1024
#define MAX_STRING_SIZE 256
#define SIZE_OF_GET 18


int fp;
int sockfd;
char msg[MAX_STRING_SIZE];
char host[MAX_STRING_SIZE];
char path[MAX_STRING_SIZE];
char buffer[BUFFER_SIZE];

/*
* @brief Funcao verifica se path leva a arquivo ou a um diretorio
*
* Se o path dado pelo usuario leva a um arquivo (por exemplo .../image.png),
* nao adiciona ''/'' no final da URL. Se leva a um diretorio (por exemplo 
* .../Imagens), adiciona ''/'' no final da URL.
*
* @param input_path URL dada pelo usuario
* @return 0 se e diretorio, 1 se e arquivo
*/

int is_file(char *input_path) {
  char *last_token = strrchr(input_path, '/');
  if (last_token != NULL)
  {
    if (strstr(last_token+1, "."))
      return 1;
  }
  return 0;
}

/**
 * @brief Funcao constroi o host e o path a partir do input dado pelo
 *        usuario no segundo argumento.
 *
 * Na URL, o host e uma string que esta entre ''https://'' ou ''http://''
 * e barra (''/''). Ou ele e a primeira string antes de uma barra. O path
 * e toda a string informada pelo usuario, com adicao de barra no final,
 * se nao houver.
 *
 * @param input_path URL dada pelo usuario
 */
void build_host_and_path(char *input_path)
{
  int i = 0, path_size = strlen(input_path);
  
  /* Add ''/'' to path if it is not the last character. */
  if ((input_path[path_size - 1] != '/') && (!is_file(input_path)))
  {
    input_path[path_size] = '/';
    input_path[path_size + 1] = '\0';
    path_size++;
  }
  strncpy(path, input_path, path_size);
  
  /*
   *  Host is string either in between ''/'' or it ends with a ''/''.
   */
  if (strstr(path, "http://") || strstr(path, "https://"))
  {
    strtok(input_path, "/");
    strcpy(host, strtok(NULL, "/"));
  } 
  else
  {
    do
    {
      host[i] = input_path[i];
      i++;
    } while (input_path[i] != '/');
  }
  if (path[path_size - 1] == '/') path[path_size - 1] = '\0';
  return;
}

/**
 * @brief Faz conexao com socket, busca pelo host e conecta-se com servidor.
 * 
 * A funcao armazena um descritor de sockete em sockfd depois tenta buscar
 * um servidor mapeado pelo nome do host. Se o servidor a o sockfd existem,
 * sao utilizados o endereco do host e o sockfd para fazer conexao com o
 * servidor.
 *
 * @return Retorna se as conexoes deram certo,
 * finaliza programa se alguma deu errado. 
 */

void connect_to_server()
{  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if ( sockfd < 0)
  {
    perror("\nError trying to get socket");
    exit(1);
  }
  printf("Socket : %d\n", sockfd);
  
  struct addrinfo ip_hints;
  struct addrinfo *server_info, *s;
  int err;
  
  /*
   *  Find n structs of type addrinfo that matches hostname
   *  on protocols http or https

   */
  
  memset(&ip_hints, 0, sizeof(ip_hints));
  ip_hints.ai_family = AF_INET;
  ip_hints.ai_socktype = SOCK_STREAM;

  if ((err = getaddrinfo(host, "http", &ip_hints, &server_info)) != 0)
  {
    printf("\nError trying to find hostname: %s\n", gai_strerror(err));
    exit(1);
  }

  /* Get first addrinfo from the list on server_info and
   * copy it to server_addr. Copy servers IP adderss to ip
   */
  int connected = 0;  
  for (s = server_info; s != NULL; s = s->ai_next)
  {
    ((struct sockaddr_in *)s)->sin_port = htons(PORT);
     printf("hostname: %s\n", 
            inet_ntoa(((struct sockaddr_in*)s->ai_addr)->sin_addr));

     if (connect(sockfd, (struct sockaddr *)s->ai_addr, sizeof(*s->ai_addr))
         >= 0)
     {
       connected = 1;
       break;
     }
  }
  if (connected == 0)
  {
    perror("\nError trying to connect to server");
    exit(1);
  }

  freeaddrinfo(server_info);
  return;
}

/**
 * @brief Envia GET para servidor
 *
 * Funcao envia mensagem construida na main em blocos de tamanho
 * BUFFER_SIZE na sockete sockfd.
 * 
 * @return Retorna se mensagem foi enviada corretamente,
 * finaliza se funcao send nao funcionou
 */
void send_message()
{
  int bytes_sent, offset, msg_size;

  msg_size = strlen(msg);
  offset = 0;
  memset(buffer, 0, BUFFER_SIZE);
  do
  {
    strncpy(buffer, msg + offset, BUFFER_SIZE);
    bytes_sent = send(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_sent < 0)
    {
      perror("\nError trying to send message from server");
      exit(1);
    }
    else if (bytes_sent == 0)
      break;
    
    offset += bytes_sent;
  } while (offset < msg_size);
 
  return;
}

 /**
  * @brief Recebe resposta do servidor
  *
  * Funcao recebe mensagem na sockete em blocos de tamanho BUFFER_SIZE.
  * Se mensagem for header, armazena em variavel header de tamanho
  * HEADER_BUFFER_SIZE. Se a mensagem for conteudo, escreve no arquivo
  * ''file_name'', informado pelo usuario
  *
  * @return Retorna se mensagem foi recebida corretamente,
  * finaliza se funcao recv nao funcionou
  */
void recv_message()
{
  int bytes_recv;

   /* Receive header in blocks of size BUFFER_SIZE and store it on variable
   * header of size HEADER_BUFFER_SIZE. If text ''\r\n\r\n'' is found on  
   * header, everything after that is written on file ''file_name'' and
   * while loop stops. */

  int blocks = 0, still_header = 1;
  char header[HEADER_BUFFER_SIZE], *end_of_header;
  char content_in_buffer[BUFFER_SIZE];
 
  memset(header, 0, HEADER_BUFFER_SIZE);
  memset(content_in_buffer, 0, BUFFER_SIZE);
  memset(buffer, 0, BUFFER_SIZE); 
  do
  {
    bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_recv < 0)
    {
      perror("\nError trying to receive message from server");
      exit(1);
    }
    else if (bytes_recv == 0)
      break;

    strncat(header, buffer, bytes_recv);
    end_of_header = strstr(header, "\r\n\r\n");
    blocks++;
    
    if (end_of_header)
    { 
      end_of_header += 4;
      int j = 0;
      int pos = end_of_header - header;
      int pos_last_bytes = pos - BUFFER_SIZE*(blocks - 1);
      if (bytes_recv > pos_last_bytes)
      { 
        for (j = 0; j < (bytes_recv - pos_last_bytes); j++)
        {
          content_in_buffer[j] = buffer[pos_last_bytes + j];
        }
        write(fp, content_in_buffer, j);
      }
      still_header = 0;
    } 
 
  } while (still_header == 1);

  /*
   * Receive content in blocks of BUFFER_SIZE
   * and write it on file ''file_name''.
   */
  do
  {
    bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_recv < 0)
    {
      perror("\nError trying to receive message from server");
      exit(1);
    }
    write(fp, buffer, bytes_recv);
    printf("%s", buffer);

  } while (bytes_recv > 0);
  
  return;
}
  
int main(int argc, char *argv[])
{
  FILE *file_check;
  if (argc < 3)
  {
    printf("\nEnter <URL> <file name> <-s to overwrite file>\n\n");
    exit(1);
  }
  
  file_check = fopen(argv[2], "r+");
  if (file_check)
  {
    if (!argv[3] || (strncmp(argv[3], "-s", 2) != 0))
    {
      printf("\nFile ''%s'' already exists.\n", argv[2]);
      printf("To overwrite it, add the flag ''-s'' to the args.\n\n");
      exit(1);
    }
    fclose(file_check);
  }
 
  fp = open(argv[2], O_CREAT|O_RDWR|O_TRUNC, 0644);
  if (fp < 0)
  {
    perror("\nError opening the file");
    exit(1);
  }
 
  build_host_and_path(argv[1]);
  snprintf(msg, SIZE_OF_GET + strlen(path), "GET %s HTTP/1.0\r\n\r\n", path);
  connect_to_server();
  send_message(); 
  recv_message();
  close(sockfd); 
  return 0;
}
