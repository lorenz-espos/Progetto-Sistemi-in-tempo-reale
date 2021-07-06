//---------------- MONITOR.H ----------------------- 

#ifndef HEADERS_H
#define HEADERS_H

#define STACK_SIZE 10000
#define SLEEP_TIME_S 1 

#define MAILBOX_ID "MyMailbox"
#define MAILBOX_SIZE 256
#define MAILBOX_ID2 "MyMailbox2"

#define TICK_TIME 250000000

#define ALTITUDE_SIZE 5
#define SPEED_SIZE 3
#define TEMP_SIZE 3

#define RAW_SEN_SHM 121111
#define PROC_SEN_SHM 121112


struct raw_sensors_data
{
    unsigned int altitudes[ALTITUDE_SIZE]; 	//uptated every 250ms
    unsigned int speeds[SPEED_SIZE];		//updated every 500ms
    int temperatures[TEMP_SIZE];		//uptaded every	second
    SEM* mutex;
};

struct processed_sensors_data
{
    unsigned int altitude;
    unsigned int speed;
    int temperature;
    SEM* mutex2;
};

#endif
