void make_post_header(){
  char str_tmp[128];
  memset(str_post,0,sizeof(str_post));
  strncpy(str_post, "uptime=", sizeof(str_post)-1);
  strncat(str_post, str_uptime, sizeof(str_post)-1);
  
  if (strlen(ups_name) > 0) {
    memset(str_tmp,0,sizeof(str_tmp));
    strncat(str_post, "&name=", sizeof(str_post)-1);
    strncat(str_post, ups_name, sizeof(str_post)-1);
  } 
  
  if (ups_model.length() > 0) {
    memset(str_tmp,0,sizeof(str_tmp));
    ups_model.toCharArray(str_tmp,sizeof(str_tmp)-1);
    strncat(str_post, "&model=", sizeof(str_post)-1);
    strncat(str_post, str_tmp, sizeof(str_post)-1);
  }
    
  if ( first_report ) {
    strncat(str_post, "&boot=1", sizeof(str_post)-1);
    first_report = false;
  }
}  

void send_alarm_ab_input( bool wtf ){
  make_post_header();
  if ( wtf ) {
    strncat(str_post, "&alarm=\"no input voltage\"", sizeof(str_post)-1);
  } else {
    strncat(str_post, "&alarm=\"return from fail state\"", sizeof(str_post)-1);
  }

#ifdef DBG_WIFI
  CONSOLE.print("Alarm: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
  send_data();
}

void send_alarm_ab_shutdown( int status ) {
  make_post_header();
  if ( status == 50 ) {
    strncat(str_post, "&alarm=\"low battery, shutdown\"", sizeof(str_post)-1);
  } else {
    strncat(str_post, "&alarm=\"battery level fall to poweroff threshold\"", sizeof(str_post)-1);
  }

#ifdef DBG_WIFI
  CONSOLE.print("Alarm: \""); CONSOLE.print(str_post); CONSOLE.println("\"");
#endif
  send_data();
}

void usual_report(){
  char str_batt_volt[16];
  char str_tmp[128];

  make_post_header();
  if ( ups_comm ) {
    memset(str_tmp,0,sizeof(str_tmp));
    memset(str_batt_volt,0,sizeof(str_batt_volt));
    dtostrf(ups_data[0],1,2,str_batt_volt);
    sprintf(str_tmp,"&data=%s,%i,%i,%i,%i,%i", str_batt_volt, int(round(ups_data[1])), int(round(ups_data[2])), int(round(ups_data[3])), int(round(ups_data[4])), int(round(ups_data[5])) );
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

