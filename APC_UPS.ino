void read_ups() {
  char inChar[2] = {0};  // because strncpy/strncat needs \0-terminated 2nd argument
  while (UPS.available()) {
    inChar[0] = UPS.read();
    if ( is_alert( inChar[0] ) ) {
      continue;
    } else if ( ( inChar[0] == '\n' && strlen(in_str) > 0 ) || ( strlen(in_str) >= (sizeof(in_str)-1) ) ) {
      handle_answer_from_ups();
    } else {
      if ( ups_init_sent || ups_get_model_sent || ups_shutdown_sent || ups_abort_shutdown_sent ) {
        if ( isPrintable(inChar[0]) ) {
          strncat( in_str, inChar, sizeof(in_str)-1 );
        }
      } else if ( isdigit(inChar[0]) || inChar[0] == '.' ) {
         strncat( in_str, inChar, sizeof(in_str)-1 );
      }
    }
  }
}

void handle_answer_from_ups() {
  if ( ups_init_sent ){
    if ( strcmp( in_str, "SM" ) == 0 ) {
      ups_init = false;
      ups_comm = true;
      ups_incorrect_answer = false;
    } else {
      ups_incorrect_answer = true;
    }
    ups_init_sent = false;
#ifdef DEBUG_UPS
    CONSOLE.print( "UPS answered after init: \"" ); CONSOLE.print( in_str );CONSOLE.println( "\"" );
#endif
  } else if ( ups_get_model_sent ) {
    strncpy( ups_model, in_str, sizeof(ups_model)-1 );
    ups_get_model = false;
    ups_get_model_sent = false;
#ifdef DEBUG_UPS
    CONSOLE.print( "UPS model: \"" ); CONSOLE.print( in_str ); CONSOLE.println( "\"" );
#endif
  } else if ( ups_shutdown_sent ) {
#ifdef DEBUG_UPS
    CONSOLE.print( "UPS answered after shutdown: \"" ); CONSOLE.print( in_str ); CONSOLE.println( "\"" );
#endif
    ups_shutdown = false;
    ups_shutdown_sent = false;
    if ( standalone == 0 ) {
      send_alarm_ab_shutdown( true );
      after_party++;
      eeprom_save();
    }
  } else if ( ups_abort_shutdown_sent ) {
#ifdef DEBUG_UPS
    CONSOLE.print( "UPS answered after abort shutdown: \"" ); CONSOLE.print( in_str ); CONSOLE.println( "\"" );
#endif
    ups_abort_shutdown = false;
    ups_abort_shutdown_sent = false;
    ups_go_2_shutdown = false;
    if ( standalone == 0 ) {
      send_alarm_ab_shutdown( false );
      if ( after_party != 0 ) {
        after_party=0;
        eeprom_save();
      }
    }
  } else {
    convr[ups_cmd_count]( in_str );
#ifdef DEBUG_UPS
    CONSOLE.print(ups_desc[ups_cmd_count]); CONSOLE.print(": "); CONSOLE.print(in_str); CONSOLE.println("; ");
#endif
  }
  memset( in_str, 0, sizeof(in_str) );
  ups_cmd_sent = false;
}

void ups_send_cmd() {
  char och;
  if ( ups_cmd_sent ) {
    if ( ++ups_sent_tries <= MAX_UPS_SENT_TRIES ) {
      return;
    } else {
      ups_init = true;
      ups_init_sent = false;
      ups_get_model = true;
      ups_get_model_sent = false;
      ups_shutdown = false;
      ups_shutdown_sent = false;
      ups_abort_shutdown = false;
      ups_abort_shutdown_sent = false;
      ups_sent_tries=0;
      ups_incorrect_answer = false;
      memset(ups_model, 0, sizeof(ups_model));
      ups_comm=false;
 #ifdef DEBUG_UPS
      CONSOLE.println("no answer from UPS");
#endif
    }
  }

  if ( ups_init ) {
    och = 'Y';
    ups_init_sent = true;
#ifdef DEBUG_UPS
    CONSOLE.println("sent init command");
#endif
  } else if ( ups_get_model ) {
    och = '\x1';  // Ctrl+A
    ups_get_model_sent = true;
#ifdef DEBUG_UPS
    CONSOLE.println("ask UPS model");
#endif
  } else if ( ups_shutdown ) {  // it is special command
    och = 'S';
    ups_shutdown_sent = true;
#ifdef DEBUG_UPS
    CONSOLE.println("sent shutdown command");
#endif
  } else if ( ups_abort_shutdown ) {  // it is special command
    och = '\x7f';
    ups_abort_shutdown_sent = true;
#ifdef DEBUG_UPS
    CONSOLE.println("sent abort shutdown command");
#endif
  } else {
    if ( ++ups_cmd_count >= ups_cmd_allcount ) {
      ups_cmd_count=0;
    }
    och = ups_cmd[ups_cmd_count];
#ifdef DEBUG_UPS
    CONSOLE.print("sent command N");
    CONSOLE.print(ups_cmd_count);
    CONSOLE.print(" \"");
    CONSOLE.print(ups_cmd[ups_cmd_count]);
    CONSOLE.println("\"");
#endif
  }
  UPS.print( och );
  ups_cmd_sent = true;
  just_boot = false;
}

void conv_battery_volt( char* ch ) {
  battery_voltage = atof( ch );
}

void conv_battery_level( char* ch ) {
  battery_level = int( round( atof( ch ) ) );
}

void conv_temp_intern( char* ch ) {
  temp_intern = atof( ch );
}

void conv_line_voltage( char* ch ) {
  line_voltage = int( round ( atof( ch ) ) );
}

void conv_power_load( char* ch ) {
  power_load = atof ( ch );
}

void conv_status( char* ch ) {
  ups_status = strtoul( ch, NULL, 16 );
}

bool is_alert( char ch ) {
  switch ( ch ) {
    case '!': // line fail
            if  ( ! ( ups_status & 0x10 ) ) {
              ups_status = ups_status | 0x10;
              ups_status = ups_status & ( ~ 0x8 );
              send_alarm_ab_input( true );
            }
#ifdef DEBUG_UPS
            CONSOLE.println("No Input voltage");
#endif
            break;
    case '$': // return from line fail
            if ( ups_status & 0x10 ) {
              ups_status = ups_status | 0x8;
              ups_status = ups_status & ( ~ 0x10 );
              send_alarm_ab_input( false );
            }
#ifdef DEBUG_UPS
            CONSOLE.println("Return from Fail");
#endif
            break;
    case '%': // low battery
            if  ( ! ( ups_status & 0x40 ) ) {
              ups_status = ups_status | 0x40;
            }
#ifdef DEBUG_UPS
            CONSOLE.println("UPS warning - LOW battery");
#endif
            break;
    case '+': // return from low battery
            if  ( ups_status & 0x40 ) {
              ups_status = ups_status & ( ~ 0x40 );
            }
#ifdef DEBUG_UPS
            CONSOLE.println("UPS info - return from low battery");
#endif
            break;
    case '*': // about to turn off
            send_alarm_last_breath();
#ifdef DEBUG_UPS
            CONSOLE.println("UPS info - about to turn off");
#endif
            break;
    // not implemented yet
    case '?': // abnormal condition
    case '=': // return from abnormal condition
    case '#': // replace battery
    case '&': // check alarm register for fault
    case '|': // variable change in EEPROM
            break;
    default:
            return( false );
  }
  return( true );
}

int debug_count=0;  // DEBUG !!!

void check_ups_status() {

  if ( ( poweroff_threshold == 0 ) || ( ! ups_comm ) || ( ! ( ups_status & 0x18 ) ) ) {
    return;
  }
#ifdef DEBUG_UPS
      CONSOLE.println("check_ups_status  ");
      CONSOLE.print("status = ");
      CONSOLE.print(ups_status, HEX);
      CONSOLE.print("H ");
      CONSOLE.println(ups_status, BIN);
#endif

  if ( ! ( ups_status & 0x10 ) ) {
#ifdef DEBUG_UPS
      CONSOLE.println("      check_ups_status  - ON line");
#endif
    if ( ups_go_2_shutdown ) {   // power returned while UPS was in a grace period
      ups_abort_shutdown = true;
    }
    return;
  }

#ifdef DEBUG_UPS
      CONSOLE.println("      check_ups_status - on BATTERY");
#endif

  if ( ups_go_2_shutdown ) {
    return;
  }

/* DEBUG */
if ( ++debug_count > 10 ) {
  poweroff_threshold = battery_level;
}
/* DEBUG !!! */

#ifdef DEBUG_UPS
      CONSOLE.print("         check_ups_status - battery_level=");
      CONSOLE.print(battery_level);
      CONSOLE.print("   poweroff_threshold=");
      CONSOLE.print(poweroff_threshold);
      CONSOLE.print("   ups_status=");
      CONSOLE.println((ups_status & 0x50));
#endif

  if ( ( battery_level > poweroff_threshold ) && ( ( ups_status & 0x50 ) != 0x50 ) ) {
    return;
  }

#ifdef DEBUG_UPS
  if ( ups_status & 0x40 )
    CONSOLE.println("Battery low");
  else
    CONSOLE.println("Battery level fall to poweroff threshold");
#endif

  ups_shutdown = true;
  ups_go_2_shutdown = true;

}
