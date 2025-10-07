#ifndef TIMEPARSER_H
#define TIMEPARSER_H

// Error codes
#define TIME_LEN_ERROR      -1
#define TIME_ARRAY_ERROR    -2
#define TIME_VALUE_ERROR    -3

#ifdef __cplusplus
extern "C" {
#endif

int time_parse(char *time);

#ifdef __cplusplus
}
#endif

#endif
