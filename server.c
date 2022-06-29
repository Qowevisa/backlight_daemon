#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

// IBM socket example
#include <sys/socket.h>
#include <sys/un.h>

// Wait
#include <sys/wait.h>

#define SOCK_PATH  "tpf_unix_sock.server"
#define DATA "Hello from server"

int main() {
    printf("before setuid(), uid=%d, effective uid=%d\n",
         (int) getuid(), (int) geteuid());

    pid_t id = fork();
    if (id == 0) {
        // child
        printf("PID 0 : uid=%d, effective uid=%d\n",
                (int) getuid(), (int) geteuid());

        // SERVER
        int server_sock, client_sock, rc;
        socklen_t len;
        int bytes_rec = 0;
        struct sockaddr_un server_sockaddr;
        struct sockaddr_un client_sockaddr;     
        char buf[256];
        int backlog = 10;
        memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
        memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
        memset(buf, 0, 256);                
        
        /**************************************/
        /* Create a UNIX domain stream socket */
        /**************************************/
        server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_sock == -1){
            printf("SOCKET ERROR: %s\n", strerror(errno));
            exit(1);
        }

        if (setuid(1000) != 0) {
            perror("setuid() error");
        } else {
        
            /***************************************/
            /* Set up the UNIX sockaddr structure  */
            /* by using AF_UNIX for the family and */
            /* giving it a filepath to bind to.    */
            /*                                     */
            /* Unlink the file so the bind will    */
            /* succeed, then bind to that file.    */
            /***************************************/
            server_sockaddr.sun_family = AF_UNIX;   
            strcpy(server_sockaddr.sun_path, SOCK_PATH); 
            len = sizeof(server_sockaddr);
            
            unlink(SOCK_PATH);
            rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
            if (rc == -1){
                printf("BIND ERROR: %s\n", strerror(errno));
                close(server_sock);
                exit(1);
            }
        
            /*********************************/
            /* Listen for any client sockets */
            /*********************************/
            rc = listen(server_sock, backlog);
            if (rc == -1){ 
                printf("LISTEN ERROR: %s\n", strerror(errno));
                close(server_sock);
                exit(1);
            }
            printf("socket listening...\n");
        
            /*********************************/
            /* Accept an incoming connection */
            /*********************************/
            client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
            if (client_sock == -1){
                printf("ACCEPT ERROR: %s\n", strerror(errno));
                close(server_sock);
                close(client_sock);
                exit(1);
            }
            
            /****************************************/
            /* Get the name of the connected socket */
            /****************************************/
            len = sizeof(client_sockaddr);
            rc = getpeername(client_sock, (struct sockaddr *) &client_sockaddr, &len);
            if (rc == -1){
                printf("GETPEERNAME ERROR: %s\n", strerror(errno));
                close(server_sock);
                close(client_sock);
                exit(1);
            }
            else {
                printf("Client socket filepath: %s\n", client_sockaddr.sun_path);
            }
            
            /************************************/
            /* Read and print the data          */
            /* incoming on the connected socket */
            /************************************/
            printf("waiting to read...\n");
            bytes_rec = recv(client_sock, buf, sizeof(buf), 0);
            if (bytes_rec == -1){
                printf("RECV ERROR: %s\n", strerror(errno));
                close(server_sock);
                close(client_sock);
                exit(1);
            }
            else {
                printf("DATA RECEIVED = %s\n", buf);
            }
            
            /******************************************/
            /* Send data back to the connected socket */
            /******************************************/
            memset(buf, 0, 256);
            strcpy(buf, DATA);      
            printf("Sending data...\n");
            rc = send(client_sock, buf, strlen(buf), 0);
            if (rc == -1) {
                printf("SEND ERROR: %s", strerror(errno));
                close(server_sock);
                close(client_sock);
                exit(1);
            }   
            else {
                printf("Data sent!\n");
            }
            
            /******************************/
            /* Close the sockets and exit */
            /******************************/
            close(server_sock);
            close(client_sock);
            return 0;
        }
    }
    printf("PID %d : uid=%d, effective uid=%d\n",
            id, (int) getuid(), (int) geteuid());
    int status;
    wait(&status);
    printf("Status = %d\n", status);
    return 0;
}
