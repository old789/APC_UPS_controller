bool read_ups() {
  bool rc = false;
  char inChar[2] = {0};  // because strncpy/strncat needs \0-terminated 2nd argument
  while (UPS.available()) {
    inChar[0] = UPS.read();
    if ( inChar[0] == '!' ) {
      ups_alarm = true;
      send_alarm_ab_input( true );
#ifdef DEBUG_UPS
      CONSOLE.println("No Input voltage"); 
#endif
    } else if (inChar[0] == '$') {
      ups_alarm = false;
      send_alarm_ab_input( false );
#ifdef DEBUG_UPS
      CONSOLE.println("Return from Fail"); 
#endif
    } else if ( inChar[0] == '.' ){
      strncat( in_str, ",", sizeof(in_str)-1 );
     } else if ( ( inChar[0] == '\n' && strlen(in_str) > 0 ) || ( strlen(in_str) == (sizeof(in_str)-1) ) ) {
      if ( ups_init ){
#ifdef DEBUG_UPS
        CONSOLE.print( "UPS answered after init: \"" );
        CONSOLE.print( in_str );
        CONSOLE.println( "\"" );
#endif
      } else if ( ups_get_model ) {
        strncpy( ups_model, in_str, sizeof(ups_model)-1 );
        ups_comm=true;
#ifdef DEBUG_UPS
        CONSOLE.print( "UPS model: \"" );
        CONSOLE.print( in_str );
        CONSOLE.println( "\"" );
#endif
      } else if ( ups_shutdown ) {
#ifdef DEBUG_UPS
        CONSOLE.print( "UPS answered after shutdown: \"" );
        CONSOLE.print( in_str );
        CONSOLE.println( "\"" );
#endif
      } else {
        ups_data[ups_cmd_count] = atof( in_str );
#ifdef DEBUG_UPS
        CONSOLE.print(ups_desc[ups_cmd_count]);
        CONSOLE.print(": ");
        CONSOLE.print(ups_data[ups_cmd_count]);
        CONSOLE.println("; ");
#endif
      }
      memset( in_str, 0, sizeof(in_str) );
      rc = true;
      ups_cmd_sent = false;
    } else {
      if ( ups_init || ups_get_model ) {
        if ( isPrintable(inChar[0]) ) {
          strncat( in_str, inChar, sizeof(in_str)-1 );
        }
      } else if ( isdigit(inChar[0]) ) {
         strncat( in_str, inChar, sizeof(in_str)-1 );
      }
    }
  }
  return(rc);
}

void ups_send_cmd() {
  if ( ups_cmd_sent ) {
    if ( ++ups_sent_tries <= MAX_UPS_SENT_TRIES ) {
      return;
    } else {
      ups_init=true;
      ups_get_model=true;
      ups_sent_tries=0;
      memset(ups_model, 0, sizeof(ups_model));
      ups_comm=false;
 #ifdef DEBUG_UPS
      CONSOLE.println("no answer from UPS");
#endif
    }
  } else if ( ups_init ) {
    ups_init = false;
  } else if ( ups_get_model ) {
    ups_get_model = false;
  } else if ( ups_shutdown ) {
    ups_shutdown = false;
  } else if ( ++ups_cmd_count >= ups_cmd_allcount ) {
    ups_cmd_count=0;
//#ifdef DEBUG_UPS
//    CONSOLE.println("ups_cmd_count cleared");
//#endif
  }
 
  if ( ups_init ) {
    UPS.print("Y");
#ifdef DEBUG_UPS
    CONSOLE.println("sent init command");
#endif
  } else if ( ups_get_model ) {
    UPS.print("\x1");   //Ctrl+A
#ifdef DEBUG_UPS
    CONSOLE.println("ask UPS model");
#endif
  } else {
    UPS.print(ups_cmd[ups_cmd_count]);
#ifdef DEBUG_UPS
    CONSOLE.print("sent command N");
    CONSOLE.print(ups_cmd_count);
    CONSOLE.print(" \"");
    CONSOLE.print(ups_cmd[ups_cmd_count]);
    CONSOLE.println("\"");
#endif
  }
  ups_cmd_sent = true;
}
