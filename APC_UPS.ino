bool read_ups() {
  bool rc = false;
  while (UPS.available()) {
    char inChar = UPS.read();
    if ( inChar == '!' ) {
      ups_alarm = true;
      send_alarm_ab_input( true );
#ifdef DEBUG_UPS
      CONSOLE.println("No Input voltage"); 
#endif
    } else if (inChar == '$') {
      ups_alarm = false;
      send_alarm_ab_input( false );
#ifdef DEBUG_UPS
      CONSOLE.println("Return from Fail"); 
#endif
    } else if ( inChar == '.' ){
      inString += ",";
    } else if ( inChar == '\n' && inString.length() > 0 ) {
      if ( ups_init ){
#ifdef DEBUG_UPS
        CONSOLE.println("UPS answered after init: \"" + inString + "\"");
#endif
      } else if ( ups_get_model ) {
        ups_model = inString;
        ups_comm=true;
#ifdef DEBUG_UPS
        CONSOLE.println("UPS model: \"" + inString + "\"");
#endif
      } else if ( ups_shutdown ) {
#ifdef DEBUG_UPS
        CONSOLE.println("UPS answered after shutdown: \"" + inString + "\"");
#endif
      } else {
        ups_data[ups_cmd_count] = inString.toFloat();
#ifdef DEBUG_UPS
        CONSOLE.println(ups_desc[ups_cmd_count] + ": " + ups_data[ups_cmd_count] + "; ");
#endif
      }
      inString = "";
      rc = true;
      ups_cmd_sent = false;
    } else {
      if ( ups_init || ups_get_model ) {
        if ( isPrintable(inChar) ) {
          inString += inChar;
        }
      } else if ( isdigit(inChar) ) {
        inString += inChar;
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
      ups_model="";
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
    CONSOLE.println("sent command \"" + ups_cmd[ups_cmd_count] + "\"");
#endif
  }
  ups_cmd_sent = true;
}
