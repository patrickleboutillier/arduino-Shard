#ifndef SHARD_h
#define SHARD_h

#include "Arduino.h"


#define SHARD_MAX_CMD_LEN   8
#define SHARD_MAX_MONITORS  4


struct ShardLambda {
    template<typename Tret, typename T>
    static Tret lambda_ptr_exec(void* data) {
        return (Tret) (*(T*)fn<T>())(data);
    }

    template<typename Tret = void, typename Tfp = Tret(*)(void*), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, T>;
    }

    template<typename T>
    static void* fn(void* new_fn = nullptr) {
        static void* fn;
        if (new_fn != nullptr)
            fn = new_fn;
        return fn;
    }
};


#define Shard_monitor(name, expr)   {auto f = [&](void*) {return (expr) ;} ; Shard._monitor_lambda(name, ShardLambda::ptr<int>(f)) ;}


struct ShardMonitor {
  bool active ;
  char cmd[SHARD_MAX_CMD_LEN+1] ;
  int (*fptr)(void*) ;
  int result ;
} ;


class ShardImpl {
  public:
    ShardImpl() ;
    void begin(char eol = '\n') ;
    int run(const char *cmdstr) ;
    int _monitor_lambda(const char *name, int (*fptr)(void*)) ;
    void loop() ;
  private:
    byte _mon_idx ;
    ShardMonitor _mon[SHARD_MAX_MONITORS] ;
    bool _ignore ;
    bool _break ;
    bool _active ;
    char _eol ;
    bool _echo_cmd ;
    byte _cmdidx ;
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