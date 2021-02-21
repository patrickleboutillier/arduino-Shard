#include "Arduino.h"
#include "Shard.h"


// The global Shard Object is created here.
ShardImpl Shard ;


ShardImpl::ShardImpl(){
  _ignore = 0 ;
  _active = 0 ;  
  _break = 0 ;
  _mon_idx = 0  ;
  for (byte i = 0 ; i < SHARD_MAX_MONITORS ; i++){
    _mon[i].active = 0 ;
  }
  _cmdidx = 0 ;
}


void ShardImpl::begin(char eol = '\n'){
  if (Serial){
    _active = 1 ;
  }
  _eol = eol ;
  _echo_cmd = 1 ;
  Serial.println() ;
  Serial.println(F("Shard initialized. Type 'help' for command listing and information.")) ;
  prompt() ;
}


void ShardImpl::loop(){
  if (! _active){
    // Serial is not activate, we cannot do anything...
    return ;
  }

  do {
    if (! _break){
      // Run monitors if we are not in a break.
      run_monitors() ;
    }
    
    while (Serial.available()){    
      char c = Serial.read() ;
      if ((c != _eol)&&(_cmdidx >= SHARD_MAX_CMD_LEN)){
        if (! _ignore){ 
          _ignore = 1 ;
          Serial.print("Command line too long, ignored (maximum ") ;
          Serial.print(SHARD_MAX_CMD_LEN) ;
          Serial.println(" characters).") ;
          prompt() ;
        }
      }
      
      if (! _ignore){
        process_char(c) ;
      }
      else {
        if (c == _eol){
          _cmdidx = 0 ;
          _ignore = 0 ;
        }
      }
    }
  } while (_break) ;
}


void ShardImpl::run_monitors(){
  for (byte i = 0 ; i < SHARD_MAX_MONITORS ; i++){
    if (_mon[i].active) {
      int ret = execute_cmd(_mon[i].cmd, true) ;
      if (ret != _mon[i].result){
        // TODO: cleanup/make more efficient.
        Serial.print("monitor(") ;
        Serial.print(_mon[i].cmd) ;
        Serial.print("): ") ;
        Serial.print(_mon[i].result) ;
        Serial.print(" -> ") ;
        Serial.println(ret) ;
        _mon[i].result = ret ;   
      }
    }
  }
}


void ShardImpl::process_char(char c){
  if ((_cmdidx == 0)&&(c == ' ')){
    return ;
  }

  if (c == _eol){
    if (_cmdidx > 0){
      while ((_cmd[_cmdidx-1]) == ' '){
        _cmdidx-- ;
      }
    }
    _cmd[_cmdidx] = '\0' ;
    if (_echo_cmd){
      // Arduino serial monitor doesn't echo the command to the output, so we do it.
      Serial.println(_cmd) ; 
    }

    if (strlen(_cmd) != 0){
      execute_cmd(_cmd, false) ;
      prompt() ;
      _cmdidx = 0 ;
    }
    
    return ;
  }
  
  switch (c){
    case '\n':
    case '\r':
    case '\t':
      break ;
    default:
      if ((_cmdidx > 0)&&(c == ' ')&&(_cmd[_cmdidx-1] == ' ')){
        return ;
      }
      _cmd[_cmdidx++] = c ;
      break ;
  }
}


int ShardImpl::execute_cmd(const char *cmdstr, bool silent){
  char cmd[SHARD_MAX_CMD_LEN] ;
  strcpy(cmd, cmdstr) ;
  
  if ((strncmp(cmd, "mdr", 3) == 0)||(strncmp(cmd, "mar", 3) == 0)){
    // Monitor mode
    if (_mon_idx = SHARD_MAX_CMD_LEN-1){
        Serial.println("All available monitors are used.") ;
        return 0 ;
    }
    int ret = execute_cmd(cmd+1, true) ;
    if (ret != -1){ // Command succeeded
      // Place command in monitor mode
       monitor_cmd(cmd+1, ret) ;
    }
    return 0 ;
  }
  else if (strcmp(cmd, "help") == 0){
    help() ;    
    return 0 ;
  } 
  else if (strcmp(cmd, "clm") == 0){
    for (byte i = 0 ; i < SHARD_MAX_MONITORS ; i++){
      _mon[i].active = 0 ;    
    }
    Serial.println("Monitors cleared.") ;
    return 0 ;
  }
  else if (strcmp(cmd, "break") == 0){
    if (! _break){
      _break = 1 ;    
      Serial.println("Sketch interrupted.") ;
    }
    return 0 ;
  }
  else if (strcmp(cmd, "cont") == 0){
    if (_break){
      _break = 0 ;
      Serial.println("Resuming sketch.") ;  
    }
    return 0 ;
  }
  else if (strcmp(cmd, "ls") == 0){
    ls() ;
    return 0 ;
  }
  else if ((strncmp(cmd, "dr", 2) == 0)||(strncmp(cmd, "ar", 2) == 0)){
    bool analog = cmd[0] == 'a' ;
    strtok(cmd, " ") ; // dr/ar
    int pin = parse_pin(strtok(NULL, " "), analog) ;
    if (pin != -1){
      return read(pin, analog, silent) ;
    }
  }
  
  // Invalid command or argument
  Serial.print(cmdstr) ;
  Serial.print(": ") ;
  Serial.println("invalid command or argument") ;
  return -1 ;
}


void ShardImpl::monitor_cmd(const char *cmdstr, int result){
  _mon[_mon_idx].active = 1 ;
  strcpy(_mon[_mon_idx].cmd, cmdstr) ;
  _mon[_mon_idx].result = result ;  
}


int ShardImpl::read(int pin, bool analog, bool silent){
  int ret = (analog ? analogRead(pin) : digitalRead(pin)) ;
  if (! silent){
    Serial.println(ret) ;
  }
  return ret ;
}


void ShardImpl::prompt(){
  Serial.print(F("[shard]$ ")) ;
}


uint8_t getPinMode(uint8_t pin){
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  volatile uint8_t *reg, *out;
  reg = portModeRegister(port);
  out = portOutputRegister(port);

  if (*reg & bit)
    return OUTPUT;
  else if (*out & bit)
    return INPUT_PULLUP;
  else
    return INPUT;
}


void ShardImpl::ls(){
    Serial.println("PIN\tINPUT/OUTPUT\tMODE") ;
    for (int i = 0 ; i < NUM_DIGITAL_PINS ; i++){
      char buf[SHARD_MAX_CMD_LEN] ;
      strcpy(buf, "pin-") ;
      strcat(buf, (i >= A0 ? "A" : "")) ;
      char bufi[8] ;
      strcat(buf, itoa((i >= A0 ? i - A0 : i), bufi, 10)) ;
      strcat(buf, "\t") ;
      strcat(buf, (i >= A0 ? "ANALOG" : "DIGITAL")) ;
      strcat(buf, "/") ;
      strcat(buf, "DIGITAL\t") ;
            
      int mode = getPinMode(i) ;
      char *c = "INPUT" ;
      switch(mode){
        case OUTPUT:
          c = "OUTPUT" ;
          break ;
        case INPUT_PULLUP:
          c = "INPUT_PULLUP" ;
          break ;
      }
      strcat(buf, c) ;
      Serial.println(buf) ;
    }
}


const char *_helptxt[] = {
  "Shard Help",
  "ar pinN             analogRead(pinN)",
  "aw pinN value       analogWrite(pinN, value)",
  "dr pinN             digitalRead(pinN)",
  "dw pinN value       digitalWrite(pinN, value)",
  "help                Displays this help screen", 
  "ls                  List all pins, their type and there current mode",
  "pm pinN i/ip/o      pinMode(pinN, INPUT/INPUT_PULLUP/OUTPUT)",       
} ;


void ShardImpl::help(){
  for (int i = 0 ; i < (sizeof(_helptxt)/2) ; i++){
    Serial.println(_helptxt[i]) ;
  }
}


int ShardImpl::parse_int(const char *cint, byte maxlen){
  if (cint == NULL){
    return -1 ;
  }
  
  char *x = cint ;
  char buf[8] ;
  byte i = 0 ;
  while ((*x >= '0')&&(*x <= '9')){
    buf[i++] = *(x++) ;
    if (i == maxlen){
      break ;
    }
  }
  if ((i == 0)||((*x != '\0'))){
    // Int has 0 digits or string had invalid chracter
    return -1 ;
  }
  if ((i == maxlen)&&((*x >= '0')&&(*x <= '9'))){
    // Int is too long
    return -1 ;
  }
  
  buf[i] = '\0' ;
  return atoi(buf) ;
}


int ShardImpl::parse_pin(const char *cpin, bool analog_only){
  if (cpin == NULL){
    return -1 ;
  }
  
  char *x = cpin ;
  int pin = 0 ;
  if (*x == 'A'){
    pin = A0 ;
    x++ ;
  }
  else if (analog_only){
    return -1 ;
  }
  
  int n = parse_int(x, 2) ;
  if (n == -1){
    return n ;
  }
  return pin + n ;
}
