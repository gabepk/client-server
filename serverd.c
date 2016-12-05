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

#define BACKLOG 50
#define MAX_NAME_SIZE 64
#define HEADER_BUFFER_SIZE 1024
#define BUFFER_SIZE 128

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
  printf("\n> Socket description: %d", sockfd); 
 
  /*
   * Setting options for the socket 
   * SOL_SOCKET : Socket levl
   * SO_REUSABLE : Allow other sockets to bind to this port when not being used
   */
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
  {
    perror("\nError on configuring socket");
    exit(1);
  }
  printf("\n> Socket configured");
  
  /*
   * Configuring servers address
   */
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(server_addr.sin_zero), '\0', 8);
  printf("\n> Servers address configured on port: %d", port);

  /*
   * Function bind() associates a socket with an IP address and port number
   */
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
      < 0)
  {
    perror("\nError on binding");
    exit(1);
  }
  printf("\n> Address %d (localhost) bound to port %d", INADDR_ANY, port);

  /*
   * Put socket descriptor to listen for incoming connections
   * BACKLOG : Max number of pending connections
   */
  if (listen(sockfd, BACKLOG) < 0)
  {
     perror("\nError on listening");
     exit(1);
  }
  printf("\n> Server listening");

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
  char file_name[MAX_NAME_SIZE];
  strtok(headers[fd], " ");
  strcpy(file_name, strtok(NULL, " "));
  

  /*
   * Verify file requested
   */
  if (strstr(file_name, "../"))
  {
    printf("You don't have permission to access this file.");
    bytes_to_send[fd] = 0;
    return;
  }

  int f;
  f = open(file_name, O_RDONLY, 0644);
  if (f < 0)
  {
    perror("File doesn't exists");
    bytes_to_send[fd] = 0;
    return;
  }

  /*
   * Set number of bytes to send on socket
   */
  lseek(f, 0L, SEEK_END);
  bytes_to_send[fd] = lseek(f, 0l, SEEK_CUR);
  files_to_send[fd] = f;
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
  bytes_sent[fd] += BUFFER_SIZE;
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

  fd_set readfds; /* Master file descriptor set */
  fd_set master; /* Temporaries file descriptors set for function select() */

  FD_ZERO(&master);
  FD_ZERO(&readfds);  
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
    if (select (max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
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
          else
          {
            FD_SET(new_fd, &master); /* Add socket on set of sockets  */
            if (new_fd > max_fd)
              max_fd = new_fd; /* Replace max file descriptor */

            printf("\n> Server: got connection on socket %d from %s.", i, 
                   inet_ntoa(clients_addr.sin_addr));
          }
        }
      
        /* Handle data from clients */
        else
        {
          bytes_recv = recv(i, buffer, BUFFER_SIZE, 0);

          if (bytes_recv == 0) /* Client closed or connection has error */
          {

            printf("\n> Client of socket %d finished request.", i);

            if (bytes_sent[i] == 0)
              handle_client_message(i);

            else if (bytes_sent[i] < bytes_to_send[i])
              send_client_request(i);

            else {
              memset(headers[i], 0, HEADER_BUFFER_SIZE);
              bytes_sent[i] = 0;
              bytes_to_send[i] = 0;
              close(i);
              FD_CLR(i, &master); /* Remove socket from set */
            }

          }
          else if (bytes_recv < 0)
            perror("\nError trying to receive message on socket");
          else
            strncat(headers[i], buffer, bytes_recv);
        }
      } /* i on FD_ISSET */
    } /* for */
   
/*
    fp = fopen("text.txt", "r+");
    strncpy(msg, "HTTP/1.0 200 OK\r\n\r\n", 19);
    fread(content, 12, 1, fp);
    strncat(msg, content, strlen(content));
    send(new_fd, msg, strlen(msg), 0);
    close(new_fd);
*/
  } /* while */
  return 0;
}
