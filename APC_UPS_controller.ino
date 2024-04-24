//#define DEBUG_SERIAL  // because just "DEBUG" defined in PZEM004Tv30.h ( legacy :)
#define DEBUG_UPS
#define DBG_WIFI    // because "DEBUG_WIFI" defined in a WiFiClient library

#if defined ( DEBUG_UPS ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#if defined ( DBG_WIFI ) && not defined ( DEBUG_SERIAL )
#define DEBUG_SERIAL
#endif

#include <LiquidCrystal.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <EEPROM.h>

#include <SimpleCLI.h>  // https://github.com/SpacehuhnTech/SimpleCLI

#define LCD_COLS 20
#define LCD_ROWS 4
#define MAIN_DELAY 1000
#define SHORT_DELAY MAIN_DELAY/10
#define MAX_ALLOWED_INPUT 127

#define UPS Serial
#define CONSOLE Serial1

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(D1, D2, D3, D5, D6, D7);

// Create CLI Object
SimpleCLI cli;

// Create WiFi Client
WiFiClient client;

double voltage, current, power, energy, freq, pwfactor;
uint8_t input_tries=0;
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
bool eeprom_bad=false;

String inString;
String ups_desc[] = { "Bat_volt", "Int_temp", "Input_volt", "Power_loadPr", "Bat_levelPr", "Status"};
String ups_sent[] = { "B", "C", "L", "P", "f", "Q"};
String ups_model;
float ups_reciv[20];
uint8_t ups_count = 0;
uint16_t ups_maxcount = 6;
bool ups_init = true;
bool ups_alarm = false;

char str_voltage[8];
char str_current[8];
char str_power[16];
char str_energy[16];
char str_freq[8];
char str_pfactor[8];
char str_tmp[64];
char str_post[2048];


// Some default values
uint8_t def_input_delay = 3;
uint8_t def_poweroff_threshold = 50;
uint8_t def_standalone = 1;

// EEPROM data
uint16_t mark = 0x55aa;
uint8_t input_delay = 3;
uint8_t poweroff_threshold = 50;
uint8_t standalone = 1;
char ssid[33];
char passw[65];
char host[17];
uint16_t port = 80;
char uri[128];

#define PT_INPUT_DELAY      sizeof(mark)
#define PT_POWER_THRESHOLD  PT_INPUT_DELAY + sizeof(input_delay)
#define PT_STANDALONE       PT_POWER_THRESHOLD + sizeof(poweroff_threshold)
#define PT_SSID             PT_STANDALONE + sizeof(standalone)
#define PT_PASSW            PT_SSID + sizeof(passw)
#define PT_HOST             PT_PASSW + sizeof(passw)
#define PT_PORT             PT_HOST + sizeof(host)
#define PT_URI              PT_PORT + sizeof(uri)
#define PT_CRC              PT_URI + sizeof(uri)
#define SIZE_EEPROM         PT_URI + sizeof(uri) - 1 // PT_CRC d'not count

// Commands
Command cmdDelay;
Command cmdPoweroff;
Command cmdStandalone;
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
  
  lcd.begin(20, 4);
  lcd.print("Booting...");

#ifdef DEBUG_SERIAL
  CONSOLE.begin(115200);
  delay(50);
  CONSOLE.println(".\nStart debugging serial");
#endif
  
  EEPROM.begin(2048);
#ifdef DEBUG_SERIAL
  CONSOLE.println("Init EEPROM");
#endif

  if ( ! eeprom_read() ) {
    drawString(1, 2, "EEPROM failed");
    eeprom_bad=true;
  } else {
    if ( ! is_conf_correct() ) {
      drawString(1, 2, "Conf. incorrect");
      eeprom_bad=true;
    }
  }

  if ( eeprom_bad ) {
    load_defaults();
  }
  
#ifdef DEBUG_SERIAL
  CONSOLE.println("Waiting for user input");
#endif
  enable_cli = wait_for_key_pressed(input_delay);
  drawString(0, 0, "                  ");

  if ( enable_cli ) {
    // Command line mode
    drawString(0, 0, "CommandLine Mode");
    Serial.begin(115200);
    delay(50);
    SetSimpleCli();
    Serial.println("Usage:");
    Serial.println(cli.toString());
    if ( eeprom_bad ){
      Serial.println("\nEEPROM error or bad config, defaults loaded");
    }
  }else{
    // usual mode
    UPS.begin(2400);
    delay(50);
    if ( standalone == 0 ) {
      enable_collect_data=true;
      drawString(0, 0, "Network mode");
#ifdef DEBUG_SERIAL
      CONSOLE.print("Enter to network mode");
#endif
      // wifi_init();
      memset(str_post,0,sizeof(str_post));
    }else{
#ifdef DEBUG_SERIAL
      CONSOLE.print("Enter to standalone mode");
#endif
      drawString(0, 0, "Standalone mode");
    }
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
  CONSOLE.println("Round " + String(upcounter++));
#endif

  if ( ups_init ) {
    UPS.print("Y");
    delay(100);
    rc = read_ups();
    delay(100);
    UPS.print("\x1");   //Ctrl+A
    delay(400);
#ifdef  DEBUG_SERIAL
    CONSOLE.println("sent init command");
#endif
  } else {
    UPS.print(ups_sent[ups_count]);
#ifdef  DEBUG_SERIAL
    CONSOLE.println("sent command \"" + ups_sent[ups_count] + "\"");
#endif
  }
  delay(100);
  rc = read_ups();
  if (rc){
      ups_init=false;
//    draw_screen();
    if ( enable_collect_data ) {
//      collect_data();
    }
  }else{
#ifdef  DEBUG_SERIAL
    CONSOLE.println("no answer from UPS");
#endif
    ups_init=true;
    if ( enable_collect_data && ( uint8_t(str_post[0]) != 0 ) ) { // data send emergency
      //send_data();
    }else{
      //draw_screen();
    }
  }

  if ( ++ups_count >= ups_maxcount ) {
    ups_count=0;
//#ifdef  DEBUG_SERIAL
//    CONSOLE.println("ups_count cleared");
//#endif
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

void drawString( uint8_t col, uint8_t row, char *str ) {
  lcd.setCursor(col, row);
  lcd.print(str);
}

bool wait_for_key_pressed( uint8_t tries ) {
  for ( uint8_t i=0; i < tries; i++ ) {
      drawString( 0, 0, "Wait for key (" ); lcd.print(i+1); lcd.print(")");
      delay(1000);
      if ( analogRead(A0) < 500 ) {
        return(true);
      }
  }
  return(false);
}

void load_defaults(){
  input_delay=def_input_delay;
  poweroff_threshold=def_poweroff_threshold;
  standalone=def_standalone;
#ifdef DEBUG_SERIAL
  CONSOLE.println("Defaults loaded");
#endif
}
