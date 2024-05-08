void make_post_header(){
  memset(str_post,0,sizeof(str_post));
  strncpy(str_post, "uptime=", sizeof(str_post)-1);
  strncat(str_post, str_uptime, sizeof(str_post)-1);
  
  if (strlen(ups_name) > 0) {
    strncat(str_post, "&name=", sizeof(str_post)-1);
    strncat(str_post, ups_name, sizeof(str_post)-1);
  } 
  
  if (strlen(ups_model) > 0) {
    strncat(str_post, "&model=", sizeof(str_post)-1);
    strncat(str_post, ups_model, sizeof(str_post)-1);
  }
    
  if ( first_report ) {
    strncat(str_post, "&boot=1", sizeof(str_post)-1);
  }
  
  if ( after_party != 0 ) {
    strncat(str_post, "&alarm=boot after shutdown", sizeof(str_post)-1);
  }
}  

void send_alarm_ab_input( bool wtf ){
  make_post_header();
  if ( wtf ) {
    strncat(str_post, "&alarm=no input voltage", sizeof(str_post)-1);
  } else {
    strncat(str_post, "&alarm=return from fail state", sizeof(str_post)-1);
  }

#ifdef DBG_WIFI
  CONSOLE.print("Alarm: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
  send_data();
}

void send_alarm_ab_shutdown() {
  make_post_header();
  if ( ( ups_status & 0x50 ) == 0x50 ) {
    strncat(str_post, "&alarm=low battery, shutdown", sizeof(str_post)-1);
  } else {
    strncat(str_post, "&alarm=battery level fall to poweroff threshold", sizeof(str_post)-1);
  }

#ifdef DBG_WIFI
  CONSOLE.print("Alarm: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
  send_data();
}

void send_alarm_last_breath() {
  make_post_header();
  strncat(str_post, "&alarm=stopped now", sizeof(str_post)-1);
  send_data();
#ifdef DBG_WIFI
  CONSOLE.print("Alarm: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
}

void usual_report(){
  char str_batt_volt[6] = {0};
  char str_power_load[6] = {0};
  char str_temp_intern[6] = {0};
  char str_tmp[128];

  make_post_header();
  if ( ups_comm ) {
    memset(str_tmp,0,sizeof(str_tmp));
    dtostrf(battery_voltage,1,2,str_batt_volt);
    dtostrf(temp_intern,1,2,str_temp_intern);
    dtostrf(power_load,1,2,str_power_load);
    sprintf(str_tmp,"&data=%s,%s,%i,%s,%i,%02x", str_batt_volt, str_temp_intern, line_voltage, str_power_load, battery_level, ups_status );
    strncat(str_post, str_tmp, sizeof(str_post)-1);
  } else {
    strncat(str_post, "&msg=UPS not answered", sizeof(str_post)-1);
  }
#ifdef DBG_WIFI
  CONSOLE.print("Prepared data: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
  send_data();
}

void send_data(){

  if ( strlen(str_post) == 0 ){
#ifdef DBG_WIFI
    CONSOLE.println("Nothing to send");
#endif
    return;
  }

  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
#ifdef DBG_WIFI
    CONSOLE.println("WiFi Disconnected");
#endif
    wifi_init();
  }
  
#ifdef DBG_WIFI
  CONSOLE.println("Send data");
#endif

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  // Ignore SSL certificate validation
  client->setInsecure();
  
  //CONSOLE.println("HTTP client");
  HTTPClient http;

  //CONSOLE.println("http begin");
  // Your Domain name with URL path or IP address with path
  // http.begin(client, serverName);
  http.begin(*client, host, port, uri);

  if ( http_auth > 0 ) {
    http.setAuthorization(http_user, http_passw);
  }
  
  //CONSOLE.println("http header");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  //http.addHeader("Content-Type", "text/plain");
  //CONSOLE.println("http post");
  httpResponseCode = http.POST(str_post);

#ifdef DBG_WIFI
  CONSOLE.print("HTTP Response code: "); CONSOLE.println(httpResponseCode);
#endif

  // Free resources
  http.end();
  
  if ( httpResponseCode == 200 ) {
    if ( first_report ) {
      first_report = false;
    }
    if ( after_party != 0 ) {
      after_party = 0;
      eeprom_save();
    }
  }
}

void wifi_init(){
#ifdef DBG_WIFI
  CONSOLE.print("Connecting to ");
  CONSOLE.print(ssid);
  CONSOLE.println(" ...");
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passw);             // Connect to the network

  lcd.setCursor(0,1); lcd.print("WiFi connecting");

  uint16_t i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    lcd.setCursor(19,1);
    lcd.write(roller[roll_cnt++]);
    if ( roll_cnt >= sizeof(roller)-1 ) roll_cnt=0;
    delay(100);
    i++;
#ifdef DBG_WIFI
    CONSOLE.print(i); CONSOLE.print(' ');
#endif
    if ( i > 1500 ) {  // if don't connect - wait, then next try
      if ( ++wifi_tries < 3 ) {
        eeprom_save();  // remembering tries between reboots
#ifdef DBG_WIFI
        CONSOLE.print("Try ");
        CONSOLE.print(wifi_tries);
        CONSOLE.println(" connect to WiFi");
#endif
        lcd.setCursor(0,1); lcd.print("WiFi not connected  ");
        for ( i=150; i > 0; i-- ) {
          lcd.setCursor(0,2); lcd.print("Reboot after "); lcd.print(i); lcd.print("s   ");
          delay(1000);
        }
        ESP.reset();
      } else {
        wifi_tries=0;
        eeprom_save();
#ifdef DBG_WIFI
        CONSOLE.println("WiFi timeout, switch to standalone");
#endif
        standalone = 1;
        return;
      }
    }
  }
  
  if ( wifi_tries != 0 ) {
    wifi_tries=0;
    eeprom_save();
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

#ifdef DBG_WIFI
  CONSOLE.println('\n');
  CONSOLE.println("Connection established!");
  CONSOLE.print("IP address: ");CONSOLE.println(WiFi.localIP());
  CONSOLE.print("RSSI: ");CONSOLE.println(WiFi.RSSI());
#endif
}

