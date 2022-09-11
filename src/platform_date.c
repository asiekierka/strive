#include "platform.h"
#include <time.h>

void platform_get_daytime(platform_daytime_t *daytime, int64_t offset) {
    time_t tval = time(NULL) + offset;
    struct tm *tm = localtime(&tval);

    daytime->year = tm->tm_year + 1900;
    daytime->month = tm->tm_mon + 1;
    daytime->day = tm->tm_mday;
    daytime->hour = tm->tm_hour;
    daytime->minute = tm->tm_min;
    daytime->second = tm->tm_sec;
}
