typedef struct Date_Only Date_Only;
typedef struct Time_Only Time_Only;
typedef struct Date_Time Date_Time;

struct Time_Only {
    i8 hour;
    i8 minute;
    i8 second;
    i32 nanosec;
};

struct Date_Only {
    i32 year;
    i8 month;
    i8 day;
};

struct Date_Time {
	Date_Only date;
	Time_Only time;
};


Date_Time datetime_now(){
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
