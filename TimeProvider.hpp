#ifndef __TIME_PROVIDER_H__
#define __TIME_PROVIDER_H__

#include <Arduino.h>
#include <Timezone.h>

class TimeProvider
{
private:
    bool initialized      = false ;
    String  NTPServer     = "pt.pool.ntp.org";
    unsigned long NTPSync = 3600;   
    int           NTPTZ   = -3 ;                // TZ Brasília

public:
    time_t getLocalTime();
    void setNTPServer( String, unsigned long );
    void setTimeZone( int );
    void logTime();
    void setup ();
    
    String getFullTime();
    String getDate();
    String getTime();

    String getHours();
    String getMinutes();
    String getSeconds();
    //void handle();
};

extern TimeProvider timeProvider;

#endif