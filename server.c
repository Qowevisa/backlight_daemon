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

// readdir
#include <dirent.h>

#define SOCK_PATH  "tpf_unix_sock.server"

#define MSGSIZE 32

int main() {
    int p[2];
    char msg[MSGSIZE];
    if (pipe(p) < 0) {
        perror("pipe()");
        return 1;
    }
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
            return 1;
        }

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
        
        while (1) {
            memset(buf, 0, 256);

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
            // checking clinet entry
            if (strcmp(buf,"exit") == 0) {
                write(p[1], buf, MSGSIZE);
                break;
            } else if (strcmp(buf, "max") == 0) {
                write(p[1], buf, MSGSIZE);
            } else if (strcmp(buf, "min") == 0) {
                write(p[1], buf, MSGSIZE);
            } else if (strcmp(buf, "dec") == 0) {
                write(p[1], buf, MSGSIZE);
            } else if (strcmp(buf, "inc") == 0) {
                write(p[1], buf, MSGSIZE);
            }
        }
        
        /******************************/
        /* Close the sockets and exit */
        /******************************/
        close(server_sock);
        close(client_sock);
        return 0;

    } else {
        // parent

        // getting into needed dir
        if (chdir("/sys/class/backlight/") < 0) {
            perror("chdir()");
            kill(id, 9);
            return 2;
        }
        // journey
        struct dirent *entry;
        DIR *curdir;

        // Needed file that we will manipulate all the time
        FILE *brightness_file = NULL;
        unsigned int max_brightness_value = 0;
        unsigned int brightness_value = 0;

        // getting to the dir
        curdir = opendir("./");
        if (!curdir) {
            perror("opendir()");
            kill(id, 9);
            return 3;
        }
        while ((entry = readdir(curdir))) {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            if (entry->d_type == DT_LNK) {
                // get into this dir
                if (chdir(entry->d_name) < 0) {
                    perror("chdir()");
                    kill(id, 9);
                    return 4;
                }
                // yes
            }
        }
        if (closedir(curdir) == -1) {
            perror("closedir()");
            kill(id, 9);
            return 5;
        }

        // reading needed directory
        curdir = opendir("./");
        if (!curdir) {
            perror("opendir()");
            kill(id, 9);
            return 6;
        }
        while ((entry = readdir(curdir))) {
            if (entry->d_type != DT_REG) {
                continue;
            }
            if (strcmp(entry->d_name, "max_brightness") == 0) {
                FILE *mbfile = fopen("max_brightness", "r");
                if (!mbfile) {
                    perror("fopen()");
                    kill(id, 9);
                    return 7;
                }
                fscanf(mbfile, "%u", &max_brightness_value);
                fclose(mbfile);
            } else if (strcmp(entry->d_name, "brightness") == 0) {
                brightness_file = fopen("brightness", "r+");
                if (!brightness_file) {
                    perror("fopen()");
                    kill(id, 9);
                    return 8;
                }
                fscanf(brightness_file, "%u", &brightness_value);
            }
        }
        if (closedir(curdir) == -1) {
            perror("closedir()");
            kill(id, 9);
            return 9;
        }

        //
        printf("PID %d : uid=%d, effective uid=%d\n",
                id, (int) getuid(), (int) geteuid());
        while (1) {
            // main file contoller thingy
            memset(msg, 0, MSGSIZE);
            read(p[0], msg, MSGSIZE);
            if (strcmp(msg, "exit") == 0) {
                int status;
                wait(&status);
                printf("Status = %d\n", status);
                if (brightness_file) {
                    fclose(brightness_file);
                }
                break;
            } else if (strcmp(msg, "max") == 0) {
                fseek(brightness_file, 0, SEEK_SET);
                fprintf(brightness_file, "%u\n", max_brightness_value);
                fflush(brightness_file);
            } else if (strcmp(msg, "min") == 0) {
                fseek(brightness_file, 0, SEEK_SET);
                fprintf(brightness_file, "0\n");
                fflush(brightness_file);
            } else if (strcmp(msg, "dec") == 0) {
                brightness_value -= ( (unsigned int) max_brightness_value / 10);
                fseek(brightness_file, 0, SEEK_SET);
                fprintf(brightness_file, "%u\n", brightness_value);
                fflush(brightness_file);
            } else if (strcmp(msg, "inc") == 0) {
                brightness_value += ( (unsigned int) max_brightness_value / 10);
                fseek(brightness_file, 0, SEEK_SET);
                fprintf(brightness_file, "%u\n", brightness_value);
                fflush(brightness_file);
            }
        }
    }
    return 0;
}
