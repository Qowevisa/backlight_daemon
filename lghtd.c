#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
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
    snprintf(time_str, TIME_LEN - 1, "%s %u %u%u:%u%u:%u%u",
            months_names[month - 1],
            day,
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
    }

int main()
{
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
    FILE *logs = fopen(LOG_PATH);
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        LOG_STR("setsid problem")
        exit(EXIT_FAILURE);
    }



    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        LOG_STR("chdir problem")
        exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */

    /* The Big Loop */
    while (1) {
       /* Do some task here ... */
       
       sleep(30); /* wait 30 seconds */
    }
    exit(EXIT_SUCCESS);
}
