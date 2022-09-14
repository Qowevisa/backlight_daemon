#include <stdio.h>
#include <stdint.h>
#include <time.h>

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

int main()
{
    char tmp[TIME_LEN] = {0};
    pretty_time(tmp);
    printf("%s\n", tmp);
    return 0;
}
