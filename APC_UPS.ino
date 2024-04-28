bool read_ups() {
  bool rc = false;
  while (UPS.available()) {
    char inChar2 = UPS.read();
    if ( inChar2 == '!' ) {
#ifdef DEBUG_SERIAL
      CONSOLE.println("No Input voltage"); 
#endif
      ups_alarm = true;   
    } else if (inChar2 == '$') {
      ups_alarm = false;
#ifdef DEBUG_SERIAL
      CONSOLE.println("Return from Fail"); 
#endif
    } else if ( inChar2 == '.' ){
      inString += ",";
    } else if ( inChar2 == '\n' && inString.length() > 0 ) {
      if ( ups_init ){
#ifdef DEBUG_SERIAL
        CONSOLE.println("UPS answered after init: \"" + inString + "\"");
#endif
      } else if ( ups_get_model ) {
        ups_model = inString;
#ifdef DEBUG_SERIAL
        CONSOLE.println("UPS model: \"" + inString + "\"");
#endif
      } else {
        ups_reciv[ups_count] = inString.toFloat();
#ifdef DEBUG_SERIAL
        CONSOLE.println(ups_desc[ups_count] + ": " + ups_reciv[ups_count] + "; ");
#endif
      }
      inString = "";
      rc = true;
      ups_cmd_sent = false;
    } else {
      if ( ups_init || ups_get_model ) {
        if ( isPrintable(inChar2) ) {
          inString += inChar2;
        }
      } else if ( isdigit(inChar2) ) {
        inString += inChar2;
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
 #ifdef  DEBUG_SERIAL
      CONSOLE.println("no answer from UPS");
#endif
    }
  } else if ( ups_init ) {
    ups_init = false;
  } else if ( ups_get_model ) {
    ups_get_model = false;
  } else if ( ++ups_count >= ups_maxcount ) {
    ups_count=0;
//#ifdef  DEBUG_SERIAL
//    CONSOLE.println("ups_count cleared");
//#endif
  }
 
  if ( ups_init ) {
    UPS.print("Y");
#ifdef  DEBUG_SERIAL
    CONSOLE.println("sent init command");
#endif
  } else if ( ups_get_model ) {
    UPS.print("\x1");   //Ctrl+A
#ifdef  DEBUG_SERIAL
    CONSOLE.println("ask UPS model");
#endif
  } else {
    UPS.print(ups_sent[ups_count]);
#ifdef  DEBUG_SERIAL
    CONSOLE.println("sent command \"" + ups_sent[ups_count] + "\"");
#endif
  }
  ups_cmd_sent = true;
}
