#include "prelude.h"
#include <stdio.h>
#include <time.h>

#define ARENA_MEM_SIZE 4096ll
static byte memory[ARENA_MEM_SIZE];

typedef struct Date Date;
typedef struct Time Time;
typedef struct Date_Time Date_Time;

// struct Time_Duration {
// 	i64 _nsec;
// };

struct Time_Point {
	i64 seconds;
	i32 nanoseconds;
};

struct Time {
    i8 hour;
    i8 minute;
    i8 second;
    i32 nanosec;
};

struct Date {
    i32 year;
    i8 month;
    i8 day;
};

struct Date_Time {
	Date date;
	Time time;
};

Date_Time time_now(){
	Date_Time dt = {0};
	struct timespec spec_buf = {0};
	struct tm time_buf = {0};

	i32 res = clock_gettime(CLOCK_REALTIME, &spec_buf);
	debug_assert(res >= 0, "Failed to get clock");

	time_t now = time(null);
	localtime_r(&now, &time_buf);

	dt.date.year    = 1900 + time_buf.tm_year;
	dt.date.month   = 1 + time_buf.tm_mon;
	dt.date.day     = time_buf.tm_mday;
	dt.time.hour    = time_buf.tm_hour;
	dt.time.minute  = time_buf.tm_min;
	dt.time.second  = time_buf.tm_sec;
	dt.time.nanosec = spec_buf.tv_nsec;

	return dt;
}

int main(){
    Logger logger = log_create_console_logger();
	Date_Time dt = time_now();
	log_info(logger, "yo");
	printf("%02d-%02d-%02d %02d:%02d:%02d %d\n", dt.date.year, dt.date.month,dt.date.day,dt.time.hour, dt.time.minute, dt.time.second, dt.time.nanosec);
}

