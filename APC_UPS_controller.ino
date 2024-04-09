//#define DEBUG_SERIAL  // because just "DEBUG" defined in PZEM004Tv30.h
//#define DEBUG_UPS
//#define DBG_WIFI    // because "DEBUG_WIFI" defined in a WiFiClient library

#if defined ( DEBUG_UPS ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#if defined ( DBG_WIFI ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <EEPROM.h>

#define SCL_PIN SCL  // SCL pin of LCD. Default: D1 (ESP8266) or D22 (ESP32)
#define SDA_PIN SDA  // SDA pin of LCD. Default: D2 (ESP8266) or D21 (ESP32)
#define LCD_COLS 16
#define LCD_ROWS 2
#define MAIN_DELAY 1000
#define SHORT_DELAY MAIN_DELAY/10
#define MAX_ALLOWED_INPUT 127

#define UPS_SERIAL Serial
#define CONSOLE Serial1

// Create CLI Object
SimpleCLI cli;

// Create WiFi Client
WiFiClient client;

double voltage, current, power, energy, freq, pwfactor;
unsigned long upcounter=0;
unsigned long ticks_sleep=0;
unsigned long ticks_start=0;
unsigned long ticks_last=0;
bool rc=false;
uint16_t cnt=0;
uint8_t roll_cnt=0;
char roller[] = { '-', '/', '|', '\\' };
bool enable_collect_data=false;
bool enable_cli=false;

char str_voltage[8];
char str_current[8];
char str_power[16];
char str_energy[16];
char str_freq[8];
char str_pfactor[8];
char str_tmp[64];
char str_post[2048];


// EEPROM data
uint16_t mark = 0x55aa;
char ssid[33];
char passw[65];
char host[17];
uint16_t port = 80;
char uri[128];

#define PT_SSID         sizeof(mark)
#define PT_PASSW        PT_SSID + sizeof(passw)
#define PT_HOST         PT_PASSW + sizeof(passw)
#define PT_PORT         PT_HOST + sizeof(host)
#define PT_URI          PT_PORT + sizeof(uri)
#define PT_CRC          PT_URI + sizeof(uri)
#define SIZE_EEPROM     PT_URI + sizeof(uri) - 1 // PT_CRC d'not count

// Commands
Command cmdSizing;
Command cmdSsid;
Command cmdPassw;
Command cmdShow;
Command cmdHost;
Command cmdPort;
Command cmdUri;
Command cmdSave;
Command cmdReboot;
Command cmdHelp;

void setup(){
  pinMode(D5, INPUT_PULLUP);

  // initialize OLED
  //u8x8.begin();
  // u8x8.setBusClock(400000);
  //u8x8.setFont(u8x8_font_8x13_1x2_f);

  EEPROM.begin(2048);

  if ( !digitalRead(D5) ){
    // Command line mode
    enable_cli=true;
    //u8x8.drawString(0, 2, "CommandLine Mode");
    Serial.begin(115200);
    delay(50);
    Serial.println("CommandLine Mode");
    (void)eeprom_read();
    SetSimpleCli();
  }else{
    // usual mode
    if ( ! eeprom_read() ) {
      //u8x8.drawString(1, 2, "EEPROM failed");
      for(;;) delay(MAIN_DELAY);
    }
    if ( ! is_conf_correct() ) {
      //u8x8.drawString(1, 2, "Conf. incorrect");
      for(;;) delay(MAIN_DELAY);
    }
    enable_collect_data=true;
  }

#ifdef DEBUG_SERIAL
    CONSOLE.begin(115200);
    delay(50);
    CONSOLE.println(".\nStart serial");
#endif
  
  if (enable_collect_data) {
//    wifi_init();
    memset(str_post,0,sizeof(str_post));
  }

} // setup()

void loop(){
  if (enable_cli) {
    loop_cli_mode();
  }else{
    loop_usual_mode();
  }
}

void loop_usual_mode(){
  ticks_start=millis();
#if defined ( DEBUG_SENSOR ) || defined ( DBG_WIFI )
  CONSOLE.println();
#endif
#ifdef DEBUG_SERIAL
  CONSOLE.print("Round "); CONSOLE.println(upcounter++);
#endif

  rc = read_ups();
  if (rc){
//    fill_screen();
//    draw_screen();
    if ( enable_collect_data ) {
//      collect_data();
    }
  }else{
//    memset(screen_cur,0,sizeof(screen_cur));
//    strncpy(screen_cur[1]," !UPS Error!",LCD_COLS);
    if ( enable_collect_data && ( uint8_t(str_post[0]) != 0 ) ) { // data send emergency
      //send_data();
    }else{
      //draw_screen();
    }
  }
  ticks_sleep=neat_interval(ticks_start);
#ifdef DEBUG_SERIAL
  CONSOLE.print("End of loop, sleeping for ");CONSOLE.print(ticks_sleep);CONSOLE.println("ms");
#endif
  delay(ticks_sleep);
}

unsigned long neat_interval( unsigned long ticks_start ){
unsigned long ticks_now=millis();
unsigned long res = 0;

  if ( ticks_now < ticks_start ) {
    res = 0xffffffffffffffff - ticks_start + ticks_now;
  }else{
    res = ticks_now - ticks_start;
  }
  if ( res >= MAIN_DELAY ) {  // it is possible in case with net timeout
    return(SHORT_DELAY);
  }
  return( MAIN_DELAY - res );
}

