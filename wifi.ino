/*
void collect_data(){
#ifdef DBG_WIFI
  CONSOLE.print("Counter="); CONSOLE.println(cnt);
#endif
  unsigned long ticks_now=millis();

  if ( ( ticks_last > ticks_now ) && ( uint8_t(str_post[0]) != 0 ) ) {  // data send if ticks counter was overflowed
    send_data();
  }
  dtostrf(voltage,1,1,str_voltage);
  dtostrf(current,1,3,str_current);
  dtostrf(power,1,1,str_power);
  dtostrf(energy,1,3,str_energy);
  dtostrf(freq,1,1,str_freq);
  dtostrf(pwfactor,1,2,str_pfactor);
  sprintf(str_tmp,"m%u=%s,%s,%s,%s,%s,%s,%u&",cnt,str_voltage,str_current,str_power,str_energy,str_freq,str_pfactor,ticks_now);
  if ( strlen(str_post) + strlen(str_tmp) >= sizeof(str_post)-1 ) {
#ifdef DBG_WIFI
     CONSOLE.println("str_post is too short");
#endif
     cnt = measurements;
  }else{
    if (cnt > 0){
      strncat(str_post,str_tmp,sizeof(str_post)-1);
    }else{
      strncpy(str_post,str_tmp,sizeof(str_post)-1);
    }
  }
  ticks_last=ticks_now;

#ifdef DBG_WIFI
  CONSOLE.print("Length of buffer="); CONSOLE.println(strlen(str_post));
#endif

  if (++cnt >= measurements) {
    send_data();
  }
}
*/
/*
void send_data(){
#ifdef DBG_WIFI
  CONSOLE.println("Send data");
#endif
  strncpy(screen_cur[0],"Sending data...",LCD_COLS);
  draw_screen();
  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
#ifdef DBG_WIFI
    CONSOLE.println("WiFi Disconnected");
#endif
    u8x8.clearDisplay();
    memset(screen_prev,0,sizeof(screen_prev));
    wifi_init();
  }
  //CONSOLE.println("HTTP client");
  HTTPClient http;

  //CONSOLE.println("http begin");
  // Your Domain name with URL path or IP address with path
  // http.begin(client, serverName);
  http.begin(client, host, port, uri);

  // If you need server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  //CONSOLE.println("http header");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  //http.addHeader("Content-Type", "text/plain");
  //CONSOLE.println("http post");
  int httpResponseCode = http.POST(str_post);
  sprintf(screen_cur[0],"Data sent: %d",httpResponseCode);
  draw_screen();

#ifdef DBG_WIFI
  CONSOLE.print("HTTP Response code: "); CONSOLE.println(httpResponseCode);
  CONSOLE.println("Free resources");
#endif

  // Free resources
  http.end();
  memset(str_post,0,sizeof(str_post));
  cnt=0;
}
*/

void wifi_init(){
#ifdef DBG_WIFI
  CONSOLE.print("Connecting to ");
  CONSOLE.print(ssid);
  CONSOLE.println(" ...");
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passw);             // Connect to the network

  drawString(0,1,"WiFi connecting");

  uint16_t i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    lcd.setCursor(19,1);
    lcd.write(roller[roll_cnt++]);
    if ( roll_cnt >= sizeof(roller) ) roll_cnt=0;
    delay(100);
    i++;
#ifdef DBG_WIFI
    CONSOLE.print(i); CONSOLE.print(' ');
#endif
    if ( i > 3000 ) {  // if don't connect switch to standalone
#ifdef DBG_WIFI
      CONSOLE.print("WiFi timeout, switch to standalone");
#endif
      standalone = 1;
      return;
    }
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

