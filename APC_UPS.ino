bool read_ups() {
  bool rc = false;
  while (UPS.available()) {
    char inChar2 = UPS.read();
    if ( isdigit(inChar2) )
      inString += inChar2;
    if ( inChar2 == '.' )
      inString += ",";
    if ( inChar2 == '!' ) {
#ifdef DEBUG_SERIAL
      CONSOLE.println("No Input voltage"); 
#endif
      ups_alarm = true;   
    }
    if (inChar2 == '$') {
      ups_alarm = false;
#ifdef DEBUG_SERIAL
      CONSOLE.println("Return from Fail"); 
#endif
    }
    if (inChar2 == '\n' && inString.length()>0) {
      if ( ups_init ){
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
    }
  }
  return(rc);
}
