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

#include "TickTwo.h"    // https://github.com/sstaub/TickTwo

#define LCD_COLS 20
#define LCD_ROWS 4
#define MAIN_DELAY 1000
#define SHORT_DELAY MAIN_DELAY/10
#define MAX_ALLOWED_INPUT 127
#define MAX_UPS_SENT_TRIES 3

#define UPS Serial
#define CONSOLE Serial1

void ups_send_cmd();
void lcd_fill();
void count_uptime();
void send_data();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(D1, D2, D3, D5, D6, D7);

// Create CLI Object
SimpleCLI cli;

// Create WiFi Client
WiFiClient client;

// Create timer object
TickTwo timer1( ups_send_cmd, 1000);
TickTwo timer2( lcd_fill, 4000);
TickTwo timer3( count_uptime, 1000);
TickTwo timer4( send_data, 60000);

double voltage, power;
uint8_t input_tries=0;
bool rc=false;
unsigned long upticks=0;
uint8_t roll_cnt=0;
char roller[] = { '-', '/', '|', '\\' };
bool enable_collect_data=false;
bool enable_cli=false;
bool eeprom_bad=false;

String inString;
String ups_model="";
String ups_desc[] = { "Bat_volt", "Int_temp", "Input_volt", "Power_loadPr", "Bat_levelPr", "Status"};
String ups_desc_lcd[] = { "Ub=", "Tint=", "Uin=", "P%=", "B%=", "Status"};
String ups_cmd[] = { "B", "C", "L", "P", "f", "Q"};
float ups_data[6]={0};
uint8_t ups_cmd_allcount = 6;
uint8_t ups_cmd_count = 0;
uint8_t ups_sent_tries = 0;
bool ups_init = true;
bool ups_get_model = true;
bool ups_alarm = false;
bool ups_alarm_prev = false;
bool ups_comm = false;
bool ups_comm_prev = false;
bool ups_cmd_sent  = false;
uint8_t screen = 0;
int httpResponseCode = 0;
char str_post[1024];

// Some default values
uint8_t def_input_delay = 3;
uint8_t def_poweroff_threshold = 50;
uint8_t def_standalone = 1;

// EEPROM data
uint16_t mark = 0x55aa;
uint8_t input_delay = 3;
uint8_t poweroff_threshold = 50;
uint8_t standalone = 1;
char ups_name[33];
char ssid[33];
char passw[65];
char host[17];
uint16_t port = 80;
char uri[128];

#define PT_INPUT_DELAY      sizeof(mark)
#define PT_POWER_THRESHOLD  PT_INPUT_DELAY + sizeof(input_delay)
#define PT_STANDALONE       PT_POWER_THRESHOLD + sizeof(poweroff_threshold)
#define PT_UPS_NAME         PT_STANDALONE + sizeof(standalone)
#define PT_SSID             PT_UPS_NAME + sizeof(ups_name)
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
Command cmdUpsName;
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
    drawString(0, 0, "Initializing...");
    UPS.begin(2400);
    delay(50);
    timer1.start();
    timer2.start();
    timer3.start();
    if ( standalone == 0 ) {
#ifdef DEBUG_SERIAL
      CONSOLE.println("Enter to network mode");
#endif
      timer4.start();
      wifi_init();
    }
#ifdef DEBUG_SERIAL
    else{
      CONSOLE.println("Enter to standalone mode");
    }
#endif

    UPS.print("Y");
    ups_cmd_sent = true;
#ifdef  DEBUG_SERIAL
    CONSOLE.println("sent init command from setup");
#endif
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

  rc = read_ups();

  timer1.update();
  timer2.update();
  timer3.update();
  if ( standalone == 0 )
    timer4.update();
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

void lcd_fill(){
  lcd.clear();
  if ( screen == 0 ) {

    if ( ! ups_comm ) {
      lcd.print("UPS not answered");
    } else {
      lcd.print(ups_model);
      lcd.setCursor(0,1);
      lcd_print_status(ups_data[5]);
      lcd.setCursor(0,2);
      lcd.print( ups_desc_lcd[4] + int(round(ups_data[4])) + " " + ups_desc_lcd[0] + ups_data[0] + "V" );
      lcd.setCursor(0,3);
      lcd.print( ups_desc_lcd[2] + int(round(ups_data[2])) + "V " + ups_desc_lcd[3] + int(round(ups_data[3])) );
    }

  }else{

    if ( standalone ) {
      lcd.print( "Standalone mode" );
      lcd.setCursor(0,1);
      lcd.print( "Uptime: ");
      lcd.print( upticks );
      lcd.setCursor(0,2);
      lcd.print( ups_desc_lcd[1] + int(round(ups_data[1])) );
    } else {
      lcd.print( "Network mode" );
      lcd.setCursor(0,1);
      lcd.print( "IP: " );
      lcd.print( WiFi.localIP() );
      lcd.setCursor(0,2);
      lcd.print( "Last answer: " );
      lcd.print( httpResponseCode );
      lcd.setCursor(0,3);
      lcd.print( ups_desc_lcd[1] + int(round(ups_data[1])) );
    }
  }

  screen = screen ^ 1;
}

void lcd_print_status( float status ) {
  String s;
  switch ( int(round(status)) ) {
    case 0: s = "OFF line"; break;
    case 8: s = "ON line"; break;
    case 10: s = "On battery"; break;
    case 50: s = "On battery - LOW"; break;
    default: s = "Status unknown";
  }
  lcd.print(s);
}

void count_uptime() {
  upticks++;
}