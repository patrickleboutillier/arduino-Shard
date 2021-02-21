#ifndef SHARD_h
#define SHARD_h

#include "Arduino.h"


#define SHARD_MAX_CMD_LEN  8


typedef struct ShardMonitor {
  bool active ;
  char cmd[SHARD_MAX_CMD_LEN] ;
  int result ;
} ;


class ShardImpl {
  public:
    ShardImpl() ;
    void begin(char eol = '\n') ;
    void loop() ;   
  private:
    ShardMonitor _mon ;
    bool _ignore ;
    bool _break ;
    bool _active ;
    char _eol ;
    bool _echo_cmd ;
    int _cmdidx ;
    char _cmd[SHARD_MAX_CMD_LEN] ;
    void process_char(char c) ;
    int execute_cmd(const char *cmdstr, bool silent) ;
    void monitor_cmd(const char *cmdstr, int result) ;
    void run_monitors() ;
    int read(int pin, bool analog, bool silent) ;
    void ls() ;
    void help() ;
    void prompt() ;
    int parse_int(const char *cint, byte maxlen) ;
    int parse_pin(const char *cpin, bool analog_only) ;
} ;


// The global Shard Object is exported here
extern ShardImpl Shard ;


#endif