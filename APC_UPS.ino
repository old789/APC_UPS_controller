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
    } else {
      if ( ups_init ) {
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
