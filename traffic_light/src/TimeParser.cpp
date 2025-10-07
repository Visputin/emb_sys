#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "timeparser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {
    // Check that the pointer is not NULL
    if (time == NULL) {
        return TIME_ARRAY_ERROR;
    }

    // Check that the string length is 6
    if (strlen(time) != 6) {
        return TIME_LEN_ERROR; 
    }

    // Check that all characters are digits
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_VALUE_ERROR;
        }
    }

    // Get hour, minute, and second parts
    char hh[3] = { time[0], time[1], '\0' };
    char mm[3] = { time[2], time[3], '\0' };
    char ss[3] = { time[4], time[5], '\0' };

    int hour = atoi(hh);
    int minute = atoi(mm);
    int second = atoi(ss);

    // Check that the values are within valid range
    if (hour < 0 || hour > 23) return TIME_VALUE_ERROR;
    if (minute < 0 || minute > 59) return TIME_VALUE_ERROR;
    if (second < 0 || second > 59) return TIME_VALUE_ERROR;

    // Calculate total seconds
    int total_seconds = hour * 3600 + minute * 60 + second;

    // Return the total seconds
    return total_seconds;
}
