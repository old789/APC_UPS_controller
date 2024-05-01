void send_data(){
  char str_tmp[16];

  memset(str_post,0,sizeof(str_post));
  memset(str_tmp,0,sizeof(str_tmp));
  if ( ups_comm ) {
    dtostrf(ups_data[0],1,2,str_tmp);
    sprintf(str_post,"m%u=%s,%i,%i,%i,%i,%i",upticks, str_tmp, int(round(ups_data[1])), int(round(ups_data[2])), int(round(ups_data[3])), int(round(ups_data[4])), int(round(ups_data[5])) );
  } else {
    strncpy(str_post, "UPS not answered", sizeof(str_post));
  }
#ifdef DBG_WIFI
  CONSOLE.print("Prepared data: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif

  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
#ifdef DBG_WIFI
    CONSOLE.println("WiFi Disconnected");
#endif
    wifi_init();
  }
  
  if ( strlen(str_post) == 0 ){
#ifdef DBG_WIFI
    CONSOLE.println("Nothing to send");
#endif
    return;
  }

#ifdef DBG_WIFI
  CONSOLE.println("Send data");
#endif

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
  httpResponseCode = http.POST(str_post);

#ifdef DBG_WIFI
  CONSOLE.print("HTTP Response code: "); CONSOLE.println(httpResponseCode);
#endif

  // Free resources
  http.end();
}

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

