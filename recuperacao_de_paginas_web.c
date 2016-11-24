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
#include <fcntl.h>

#define PORT 80
#define BUFFER_SIZE 100
#define HEADER_BUFFER_SIZE 512
#define MAX_STRING_SIZE 256
#define SIZE_OF_GET 18


int fp;
int sockfd;
char file_name[MAX_STRING_SIZE];
char msg[MAX_STRING_SIZE];
char host[MAX_STRING_SIZE];
char path[MAX_STRING_SIZE];
char buffer[BUFFER_SIZE];

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
 *
 */
void build_host_and_path (char *input_path)
{
  int i = 0, path_size = strlen(input_path);
  
  /* Add ''/'' to path if it is not the last character. */
  if (input_path[path_size - 1] != '/')
  {
    input_path[path_size] = '/';
    input_path[path_size + 1] = '\0';
    path_size++;
  }
  strncpy(path, input_path, path_size);
  
  /* Host is string either in between ''/'' or it ends with a ''/''. */
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
 
  printf("\n> Host: ''%s''\n> Path: ''%s''\n\n", host, path);  

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
 * 
 */

void connect_to_socket ()
{
  struct sockaddr_in server_addr;
  struct hostent *server;
  
  printf("> Creating Socket . . .\n");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if ( sockfd < 0)
  {
    perror("\nError trying to get socket");
    exit(1);
  }
  printf("> Socket created with success!\n");

  printf("> Searching host by name . . .\n");
  server = gethostbyname(host);
  if (!server)
  {
    herror("\nError trying to get server from host");
    exit(1);
  }
  printf("> Host found!\n");

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  
  printf("> Connecting to server . . .\n");
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) 
      < 0)
  {
    perror("\nError trying to connect to server");
    exit(1);
  }
  printf("> Server connected with success!\n");
  
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
 *
 */
void send_message()
{
  int bytes_sent, offset, msg_size;

  msg_size = strlen(msg);
  offset = 0;
  printf("\n[Message sent] \t");

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
    printf("%s", buffer);
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
  *                                                                              
  */  
void recv_message()
{
  int bytes_recv;

  fp = open(file_name, O_CREAT | O_WRONLY, "a+");                                                     
  if (fp < 0)
  {
    perror("\nError opening the file");                                         
    exit(1);                                                                    
  } 

  /* Receive header in blocks of size BUFFER_SIZE and store it on variable 
   * header of size HEADER_BUFFER_SIZE. If text ''\r\n\r\n'' is found on  
   * header, everything after that is written on file ''file_name'' and  
   * while loop stops. */

  int i = 0, blocks = 0, still_header = 1;
  char header[HEADER_BUFFER_SIZE], *end_of_header;
  char content_in_buffer[BUFFER_SIZE];
 
  memset(header, 0, HEADER_BUFFER_SIZE);
  do
  {
    memset(buffer, 0, BUFFER_SIZE);                                             
    bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);                          
    if (bytes_recv < 0)
    {
      perror("\nError trying to receive message from server");                  
      exit(1);                                                                  
    }                                                                           
    else if (bytes_recv == 0)                                                 
      break;                                                                    

    strncat(header, buffer, BUFFER_SIZE);    
    end_of_header = strstr(header, "\r\n\r\n");
    blocks++;
    printf("%d > %s\n", blocks, header);
    if (end_of_header)
    {
      int pos = end_of_header - header + 4;

      for (i = pos; i < (BUFFER_SIZE*blocks); i++)
        content_in_buffer[i-pos] = header[i];
      
      content_in_buffer[i-pos] = '\0';
      printf("CONTENT IN BUFFER: .%s.\n", content_in_buffer);
      write(fp, content_in_buffer, 1);
      still_header = 0; 
    }
    
  } while (still_header == 1);


  /* Receive content in blocks of BUFFER_SIZE 
   * and write it on file ''file_name''. */
  
  do
  {
    memset(buffer, 0, BUFFER_SIZE);
    bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_recv < 0)
    {
      perror("\nError trying to receive message from server");
      exit(1);
    }
    else if (bytes_recv == 0)
      break;

    write(fp, buffer, strlen(buffer));
  } while (bytes_recv > 0);
  
  return;
}
  
int main (int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("\nEnter <URL> <file name> <-s to overwrite file>\n\n");
    exit(1);
  }
  
  strncpy(file_name, argv[2], strlen(argv[2]));
  /*int fp_test = open(file_name, O_CREAT | O_WRONLY | O_APPEND, "a+");
  if (fp_test >= 0)
  {
    
   */ /* If file exists and flag is not on args, or 
     * it is but it's not ''-s'', program exits.  */
/*
    if (!argv[3] || (argv[3] && strncmp(argv[3], "-s", 2) != 0))
    {
      printf("\nFile ''%s'' already exists.\n", argv[2]);
      printf("To overwrite it, add the flag ''-s'' to the args.\n\n");
      exit(1);
    }
  }*/
 
  build_host_and_path(argv[1]);
  snprintf(msg, SIZE_OF_GET + strlen(path), "GET %s HTTP/1.0\r\n\r\n", path);

  connect_to_socket();
  send_message(); 
  recv_message();
  close(sockfd); 
  return 0;
}
