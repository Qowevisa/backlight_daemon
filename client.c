/************************************************************/
/* This is a stream socket client sample program for UNIX   */
/* domain sockets. This program creates a socket, connects  */
/* to a server, sends data, then receives and prints a      */
/* message from the server.                                 */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>

#define SOCK_DIR "/tmp/backligh_controller"
#define SERVER_PATH "/tmp/backligh_controller/tpf_unix_sock.server"
#define CLIENT_PATH "/tmp/backligh_controller/tpf_unix_sock.client"

int sock_errno(void);
int unlink(const char *path);
int close(int socket);

void remove_file()
{
    if (remove(CLIENT_PATH) != 0) {
        perror("remove()");
    }
}

#define BUF_LEN 1024

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <STRING>\n", argv[0]);
        return 1;
    }
    atexit(remove_file);

    int client_sock, rc, len;
    struct sockaddr_un server_sockaddr; 
    struct sockaddr_un client_sockaddr; 
    char buf[BUF_LEN];
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
     
    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
        printf("SOCKET ERROR = %s\n", strerror(errno));
        exit(1);
    }

    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to bind to.    */
    /*                                     */
    /* Unlink the file so the bind will    */
    /* succeed, then bind to that file.    */
    /***************************************/
    client_sockaddr.sun_family = AF_UNIX;   
    strcpy(client_sockaddr.sun_path, CLIENT_PATH); 
    len = sizeof(client_sockaddr);
    
    unlink(CLIENT_PATH);
    rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
    if (rc == -1){
        printf("BIND ERROR = %s\n", strerror(errno));
        close(client_sock);
        exit(1);
    }
        
    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* for the server socket and connect   */
    /* to it.                              */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);
    rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len);
    if(rc == -1){
        printf("CONNECT ERROR = %s\n", strerror(errno));
        close(client_sock);
        exit(1);
    }
    
    /************************************/
    /* Copy the data to the buffer and  */
    /* send it to the server socket.    */
    /************************************/
    memset(buf, 0, BUF_LEN);
    strncat(buf, argv[1], BUF_LEN - 1);
    // printf("Sending data...\n");
    rc = send(client_sock, buf, strlen(buf), 0);
    if (rc == -1) {
        printf("SEND ERROR = %s\n", strerror(errno));
        close(client_sock);
        exit(1);
    }   
    else {
        // printf("Data sent!\n");
    }

    /******************************/
    /* Close the socket and exit. */
    /******************************/
    close(client_sock);
    
    return 0;
}

