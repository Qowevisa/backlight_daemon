#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

// Signal handling
#include <signal.h>

// IBM socket example
#include <sys/socket.h>
#include <sys/un.h>

// Wait
#include <sys/wait.h>

// readdir
#include <dirent.h>

#define LOG_PATH "/home/qowevisa/.logs/backligh_controller.log"
#define SOCK_DIR "/tmp/backligh_controller"
#define SOCK_PATH  "/tmp/backligh_controller/tpf_unix_sock.server"

#define MSGSIZE 32

#define TIME_LEN 64

#define LONG_MSG 256+64

static uint8_t day_in_months[] =
{
    31, // Jan
    28, // Feb
    31, // Mar
    30, // Apr
    31, // May
    30, // Jun
    31, // Jul
    31, // Aug
    30, // Sep
    31, // Okt
    30, // Nov
    31  // Dec
};

static char months_names[][4] =
{
    "Jan\0",
    "Feb\0",
    "Mar\0",
    "Apr\0",
    "May\0",
    "Jun\0",
    "Jul\0",
    "Aug\0",
    "Sep\0",
    "Okt\0",
    "Nov\0",
    "Dec\0"
};

void pretty_time(char *time_str)
{
    time_t seconds = time(NULL);
    uint16_t year  = 1970;
    uint8_t month  = 0;
    uint8_t day    = 0;
    uint8_t dif_year = seconds / (365*24*60*60);
    time_t seconds_for_year = (seconds - dif_year*365*24*60*60 - dif_year*6*60*60);
    year += dif_year;
    uint16_t days = seconds_for_year / 86400;
    while (days > day_in_months[month]) {
        days -= day_in_months[month];
        if (year % 4 == 0 && month == 1) {
            days--;
        }
        month++;
    }
    day = days;
    day++;
    month++;
    uint8_t hour   = (seconds_for_year % 86400) / 3600;
    uint8_t min    = (seconds_for_year % 3600) / 60;
    uint8_t sec    =  seconds_for_year % 60;
    hour += 3; // UTC diff
    hour %= 24;
    snprintf(time_str, TIME_LEN - 1, "%s %u%u %u%u:%u%u:%u%u",
            months_names[month - 1],
            day / 10,
            day % 10,
            hour / 10,
            hour % 10,
            min / 10,
            min % 10,
            sec / 10,
            sec % 10
          );
}

#define LOG_STR(STR) \
    if (logs) { \
        char timestamp[TIME_LEN] = {0}; \
        pretty_time(timestamp); \
        fprintf(logs, "%s :: ", timestamp); \
        fprintf(logs, STR); \
        fflush(logs); \
    }

void remove_dir()
{
    remove(SOCK_PATH);
    rmdir(SOCK_DIR);
}

void sigint_handler()
{
    exit(0);
}

int main()
{
    signal(SIGINT, sigint_handler);
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
            
    /* Open any logs here */
    FILE *logs = fopen(LOG_PATH, "a");
    LOG_STR("### START WORKING ###\n");
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        LOG_STR("setsid problem\n")
        exit(EXIT_FAILURE);
    }



    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        LOG_STR("chdir problem\n")
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */

    int p[2];
    char msg[MSGSIZE];
    if (pipe(p) < 0) {
        {
        char tmp[LONG_MSG] = {0};
        snprintf(tmp, LONG_MSG - 1,
                "Daemon initialization: pipe problem %s\n", strerror(errno));
        LOG_STR(tmp);
        }
        exit(EXIT_FAILURE);
    }
    atexit(remove_dir);
    
        pid_t id = fork();
    if (id == 0) {
        // child
        {
        char tmp[LONG_MSG] = {0};
        snprintf(tmp, LONG_MSG - 1,
                "PID 0 : uid=%d, effective uid=%d\n",
                (int) getuid(), (int) geteuid());
        LOG_STR(tmp);
        }

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
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "SOCKET ERROR: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
            exit(1);
        }

        if (setuid(1000) != 0) {
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "setuid() error: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
            return 1;
        }

        struct stat st = {0};
        if (stat(SOCK_DIR, &st) == -1) {
            mkdir(SOCK_DIR, 0777);
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
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "BIND ERROR: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
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
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "LISTEN ERROR: %s\n", strerror(errno));
                LOG_STR(tmp);
                }
                close(server_sock);
                exit(1);
            }
            LOG_STR("socket listening...\n");
        
            /*********************************/
            /* Accept an incoming connection */
            /*********************************/
            client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
            if (client_sock == -1){
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "ACCEPT ERROR: %s\n", strerror(errno));
                LOG_STR(tmp);
                }
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
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "GETPEERNAME ERROR: %s\n", strerror(errno));
                LOG_STR(tmp);
                }
                close(server_sock);
                close(client_sock);
                exit(1);
            }
            else {
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "Client socket filepath: %s\n", client_sockaddr.sun_path);
                LOG_STR(tmp);
                }
            }
        
            /************************************/
            /* Read and print the data          */
            /* incoming on the connected socket */
            /************************************/
            LOG_STR("waiting to read...\n");
            bytes_rec = recv(client_sock, buf, sizeof(buf), 0);
            if (bytes_rec == -1){
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "RECV ERROR: %s\n", strerror(errno));
                LOG_STR(tmp);
                }
                close(server_sock);
                close(client_sock);
                exit(1);
            }
            else {
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "DATA RECEIVED = %s\n", buf);
                LOG_STR(tmp);
                }
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
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "chdir(\"/sys/class/backlight/\") error: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
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
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "opendir(\"./\") error. returned 3: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
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
                    {
                    char tmp[LONG_MSG] = {0};
                    snprintf(tmp, LONG_MSG - 1,
                            "chdir(entry->d_name) error: %s\n", strerror(errno));
                    LOG_STR(tmp);
                    }
                    kill(id, 9);
                    return 4;
                }
                // yes
            }
        }
        if (closedir(curdir) == -1) {
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "closedir(curdir) error: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
            kill(id, 9);
            return 5;
        }

        // reading needed directory
        curdir = opendir("./");
        if (!curdir) {
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "opendir(\"./\") error. returned 6: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
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
                    {
                    char tmp[LONG_MSG] = {0};
                    snprintf(tmp, LONG_MSG - 1,
                            "fopen(\"max_brightness\") error: %s\n", strerror(errno));
                    LOG_STR(tmp);
                    }
                    kill(id, 9);
                    return 7;
                }
                fscanf(mbfile, "%u", &max_brightness_value);
                fclose(mbfile);
            } else if (strcmp(entry->d_name, "brightness") == 0) {
                brightness_file = fopen("brightness", "r+");
                if (!brightness_file) {
                    {
                    char tmp[LONG_MSG] = {0};
                    snprintf(tmp, LONG_MSG - 1,
                            "fopen(\"brightness\") error: %s\n", strerror(errno));
                    LOG_STR(tmp);
                    }
                    kill(id, 9);
                    return 8;
                }
                fscanf(brightness_file, "%u", &brightness_value);
            }
        }
        if (closedir(curdir) == -1) {
            {
            char tmp[LONG_MSG] = {0};
            snprintf(tmp, LONG_MSG - 1,
                    "closedir(curdir) error: %s\n", strerror(errno));
            LOG_STR(tmp);
            }
            kill(id, 9);
            return 9;
        }

        //
        {
        char tmp[LONG_MSG] = {0};
        snprintf(tmp, LONG_MSG - 1,
                "PARENT %d : uid=%d, effective uid=%d\n",
                id, (int) getuid(), (int) geteuid());
        LOG_STR(tmp);
        }
        while (1) {
            // main file contoller thingy
            memset(msg, 0, MSGSIZE);
            read(p[0], msg, MSGSIZE);
            if (strcmp(msg, "exit") == 0) {
                int status;
                wait(&status);
                {
                char tmp[LONG_MSG] = {0};
                snprintf(tmp, LONG_MSG - 1,
                        "Status = %d\n", status);
                LOG_STR(tmp);
                }
                if (brightness_file) {
                    fclose(brightness_file);
                }
                LOG_STR("### END WORK ###\n");
                break;
            } else if (strcmp(msg, "max") == 0) {
                brightness_value = max_brightness_value;
                fseek(brightness_file, 0, SEEK_SET);
                fprintf(brightness_file, "%u\n", brightness_value);
                fflush(brightness_file);
            } else if (strcmp(msg, "min") == 0) {
                brightness_value = 0;
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
}
