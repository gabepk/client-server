/*!
 * \file serverd.c
 * \brief Servidor de Paginas Web Single Threaded
 *
 * "$Id: $" 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BACKLOG 10
#define MAX_NAME_SIZE 64
#define HEADER_BUFFER_SIZE 1024
#define BUFFER_SIZE 4096

int port;
int sockfd;
char buffer[BUFFER_SIZE];

/* 
 * TODO Create struct 
 */
int bytes_sent[BACKLOG];
int bytes_to_send[BACKLOG];
int files_to_send[BACKLOG];
char headers[BACKLOG][HEADER_BUFFER_SIZE];


void connect_to_server() {
  /*
   * Get socket desciptor
   */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("\nError trying to get socket");
    exit(1);
  }
  printf("> Socket description: %d\n", sockfd); 
 
  /*
   * Setting options for the socket 
   * SOL_SOCKET : Socket levl
   * SO_REUSABLE : Allow other sockets to bind to this port when not being used
   */
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
  {
    perror("Error on configuring socket");
    exit(1);
  }
  
  /*
   * Configuring servers address
   */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(server_addr.sin_zero), '\0', 8);

  /*
   * Function bind() associates a socket with an IP address and port number
   */
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
      < 0)
  {
    perror("\nError on binding");
    exit(1);
  }
  printf("> Address %d (localhost) bound to port %d\n", INADDR_ANY, port);

  /*
   * Put socket descriptor to listen for incoming connections
   * BACKLOG : Max number of pending connections
   */
  if (listen(sockfd, BACKLOG) < 0)
  {
     perror("\nError on listening");
     exit(1);
  }
  printf("> Server listening\n");

  return;
}

/**
 * @brief Verify clients request and calculate amout of data requested
 * 
 * Function will store amount of data requested on bytes_to_send[fd],
 * where fd is the socket connected to this client.
 * The data will be sent later on chunks of BUFFER_SIZE
 *
 * If the client can't access the file, bytes_to_send is set to zero
 * and on the next iteration with fd, server will close socket
 *
 * @param fd : Clients socket
 */
void handle_client_message(int fd)
{  
  // TODO Set Content-Type: "type"

  char file_name[MAX_NAME_SIZE];
  char header[HEADER_BUFFER_SIZE];
  
  strncpy(header, headers[fd], strlen(headers[fd]));
  strtok(headers[fd], " ");

  /*
   * TODO Change from strcpy to strncpy 
   */
  //int pos_init = strtok(NULL, " /");
  //int pos_end = strtok(NULL, " ");
  strcpy(file_name, strtok(NULL, " /"));
  
  file_name[strlen(file_name)] = '\0';
  printf("[FILE NAME] : %s\n\n", file_name);

  /*
   * Verify file requested
   */
  if (strstr(file_name, "../"))
  {
    printf("[ERROR] You don't have permission to access this file.\n");
    bytes_to_send[fd] = -1;
    send(fd, "POST 403 HTTP/1.0\r\n\r\n", 21, 0); /* 403 : Forbidden*/
    return;
  }

  int f;  
  f = open(file_name, O_RDONLY, 0644);
  if (f < 0)
  {
    perror("File doesn't exists or can't be read");
    bytes_to_send[fd] = -1;
    send(fd, "POST 404 HTTP/1.0\r\n\r\n", 21, 0); /* 404 : Not found */
    return;
  }

  /*
   * Set number of bytes to send on socket
   */
  lseek(f, 0L, SEEK_END);
  bytes_to_send[fd] = lseek(f, 0l, SEEK_CUR);
  files_to_send[fd] = f;
  send(fd, "POST 200 HTTP/1.0\r\n\r\n", 21, 0); /* 200 : Accept */
  return;
}

/**
 * @brief Send data to client on chunks of BUFFER_SIZE
 *
 * This functions will be called when bytes_sent[fd] is less than 
 * bytes_to_send[fd], which means that the data the client wants
 * wasn't sent completely.
 *
 * @param fd : Clients socket
 */
void send_client_request(int fd)
{
  int msg[BUFFER_SIZE];
  lseek(files_to_send[fd], bytes_sent[fd], SEEK_SET);
  int bytes_read = read(files_to_send[fd], msg, BUFFER_SIZE);

  // TODO Calcular EOF - SEEK_CUR
  
  send(fd, msg, bytes_read, 0);
  bytes_sent[fd] += bytes_read;
  return;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("\nEnter <PORT> <Directory>\n\n");
    exit(1);
  }
 /*
  *  Reportar se arquivo nao existe OU usuario nao tem permissao pra acessa-lo
  *  Verificar se arquivo de entrada esta em um diretorio ''../''.
  */
  port = atoi(argv[1]);
  connect_to_server();

  /*
   * Structs and variables
   */
  int i;
  int bytes_recv;
  int max_fd; /* Biggest file descriptor */
  int new_fd; /* New connections on descriptor new_fd */
  struct sockaddr_in clients_addr; /* Client's address */
  socklen_t sin_size; /* Size of client's addrss*/

  fd_set master; /* Master file descriptor set */
  fd_set readfds; /* Temporaries file descriptors set for read from sockets */
  fd_set writefds; /* Temporary file descriptors set for write in sockets */

  FD_ZERO(&master);
  FD_ZERO(&readfds);  
  FD_ZERO(&writefds);
  FD_SET(sockfd, &master);

  max_fd = sockfd;

  /*
   * Initiate headers and bytes sent arrays
   */
  for (i = 0; i < BACKLOG; i++) {
    memset(headers[i], 0, HEADER_BUFFER_SIZE);
    bytes_sent[i] = 0;
    bytes_to_send[i] = 0;
    files_to_send[i] = 0;
  }

  /*
   * MAIN LOOP
   */
  while (1)
  { 
    readfds = master;
    if (select (max_fd + 1, &readfds, &writefds, NULL, NULL) < 0)
    {
      perror("\nError on select");
      exit(1);
    }
  
    for (i = 0; i <= max_fd; i++)
    {
      if (FD_ISSET(i, &readfds))
      {
        /* Handle new connection */
        if (i == sockfd)
        {
          sin_size = sizeof(clients_addr);

          if ((new_fd = accept(sockfd, (struct sockaddr *)&clients_addr,
              &sin_size)) < 0)
            perror("\nError on accepting new connection");
 
          /* Add new client */
          else
          {
            FD_SET(new_fd, &master); /* Add socket on set of sockets  */
            /* Replace max file descriptor when new_fd greater than max_fd */
            if (new_fd > max_fd)
              max_fd = new_fd; 

            printf("> Server: got connection on socket %d from %s.\n", new_fd, 
                   inet_ntoa(clients_addr.sin_addr));
          }
        }

        /* Handle data from clients */
        else
        {
          bytes_recv = recv(i, buffer, BUFFER_SIZE, 0);

          if (bytes_recv == 0) /* Client closed or connection has error */
          {
            printf("> Client of socket %d finished request.\n", i);
            close(i);
            FD_SET(i, &writefds);
            FD_CLR(i, &master);
          }
          else if (bytes_recv < 0)
            perror("\nError trying to receive message on socket");
          else {
            strncat(headers[i], buffer, bytes_recv);
            if (strstr(headers[i], "\r\n\r\n") != NULL)
            {
              FD_SET(i, &writefds);
              FD_CLR(i, &master);
            }
          }
        }
      }
      else if (FD_ISSET(i, &writefds))
      {
        /* Time to send what client requested */

        /* Verify the size of data the client requeted */
        if ((bytes_sent[i] == 0) && (bytes_to_send[i] == 0))
          handle_client_message(i);

        /* Send chunks of data of size BUFFER_SIZE */
        else if (bytes_sent[i] < bytes_to_send[i])
          send_client_request(i);

        /* Server sent all data. Remove client from set of sockets */
        else {
          memset(headers[i], 0, HEADER_BUFFER_SIZE);
          bytes_sent[i] = 0;
          bytes_to_send[i] = 0;
          close(files_to_send[i]);
          files_to_send[i] = 0;
          close(i);
          FD_CLR(i, &writefds);
        }
      }
    } /* for */
  } /* while */
  return 0;
}
