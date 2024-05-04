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
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <EEPROM.h>

#include <SimpleCLI.h>  // https://github.com/SpacehuhnTech/SimpleCLI

#include "TickTwo.h"    // https://github.com/sstaub/TickTwo

#include "uptime.h"     // https://github.com/YiannisBourkelis/Uptime-Library

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
void usual_report();
void check_ups_status();

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(D1, D2, D3, D5, D6, D7);

// Create CLI Object
SimpleCLI cli;

// Create timer object
TickTwo timer1( ups_send_cmd, 1000);
TickTwo timer2( lcd_fill, 4000);
TickTwo timer3( count_uptime, 1000);
TickTwo timer4( check_ups_status, 2500);
TickTwo timer5( usual_report, 60000);

double voltage, power;
uint8_t input_tries=0;
bool rc=false;
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
bool ups_shutdown = false;
bool ups_alarm = false;
bool ups_alarm_prev = false;
bool ups_comm = false;
bool ups_comm_prev = false;
bool ups_cmd_sent = false;
bool ups_go_2_shutdown = false;
bool first_report = true;
uint8_t screen = 0;
int httpResponseCode = 0;
char str_uptime[33];
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
char ups_name[33] = {0};
char ssid[33] = {0};
char passw[65] = {0};
char host[65] = {0};
uint16_t port = 443;
char uri[128] = {0};
uint8_t http_auth = 0;
char http_user[33] = {0};
char http_passw[33] = {0};

#define PT_INPUT_DELAY      sizeof(mark)
#define PT_POWER_THRESHOLD  PT_INPUT_DELAY + sizeof(input_delay)
#define PT_STANDALONE       PT_POWER_THRESHOLD + sizeof(poweroff_threshold)
#define PT_UPS_NAME         PT_STANDALONE + sizeof(standalone)
#define PT_SSID             PT_UPS_NAME + sizeof(ups_name)
#define PT_PASSW            PT_SSID + sizeof(ssid)
#define PT_HOST             PT_PASSW + sizeof(passw)
#define PT_PORT             PT_HOST + sizeof(host)
#define PT_URI              PT_PORT + sizeof(port)
#define PT_AUTH             PT_URI + sizeof(uri)
#define PT_HUSER            PT_AUTH + sizeof(http_auth)
#define PT_HPASSW           PT_HUSER + sizeof(http_user)
#define PT_CRC              PT_HPASSW + sizeof(http_passw)
#define SIZE_EEPROM         PT_HPASSW + sizeof(http_passw) - 1 // PT_CRC d'not count

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
Command cmdHauth;
Command cmdHuser;
Command cmdHpassw;
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
    lcd.setCursor(1, 2); lcd.print("EEPROM failed");
    eeprom_bad=true;
  } else {
    if ( ! is_conf_correct() ) {
      lcd.setCursor(1, 2); lcd.print("Conf. incorrect");
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
  lcd.setCursor(0, 0);
  lcd.print("                  ");

  if ( enable_cli ) {
    // Command line mode
    lcd.setCursor(0, 0); lcd.print("CommandLine Mode");
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
    lcd.setCursor(0, 0); lcd.print("Initializing...");
    UPS.begin(2400);
    delay(50);
    timer1.start();
    timer2.start();
    timer3.start();
    timer4.start();
    strncpy( str_uptime, "0d0h0m0s\x0", sizeof(str_uptime)-1 );
    if ( standalone == 0 ) {
#ifdef DEBUG_SERIAL
      CONSOLE.println("Enter to network mode");
#endif
      timer5.start();
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
  timer4.update();
  if ( standalone == 0 )
    timer5.update();
}

bool wait_for_key_pressed( uint8_t tries ) {
  for ( uint8_t i=0; i < tries; i++ ) {
      lcd.setCursor(0,0); lcd.print("Wait for key (" ); lcd.print(i+1); lcd.print(")");
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
      lcd.print( "Up: ");
      lcd.print( str_uptime );
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
  char s[21];
  memset(s, 0, sizeof(s));
  switch ( int(status) ) {
    case 0: strncpy( s, "OFF line", sizeof(s)-1 ); break;
    case 8: strncpy( s, "ON line", sizeof(s)-1 ); break;
    case 10: strncpy( s, "On battery", sizeof(s)-1 ); break;
    case 50: strncpy( s, "On battery - LOW", sizeof(s)-1 ); break;
    default: strncpy( s, "Status unknown", sizeof(s)-1 );
  }
  lcd.print(s);
}

void count_uptime() {
  uptime::calculateUptime();
  memset(str_uptime, 0, sizeof(str_uptime));
  sprintf( str_uptime, "%ud%uh%um%us", uptime::getDays(), uptime::getHours(), uptime::getMinutes(), uptime::getSeconds() );
}

void check_ups_status() {
  int status=int(round(status));
  int battery=int(round(ups_data[4]));
  
  if ( poweroff_threshold == 0 ) {
    return;
  }
  if ( ( status != 10 ) && ( status != 50 ) ) {
    if ( ups_go_2_shutdown ) {   // power returned while UPS was in a grace period 
      ups_go_2_shutdown = false;
    }
    return;
  }
  
  if ( ups_go_2_shutdown ) {
    return;
  }
  
  if ( ( battery >= poweroff_threshold ) && ( status != 50 ) ) {
    return;
  }
  
#ifdef DEBUG_SERIAL
  if ( status == 50 )
    CONSOLE.println("Battery low");
  else
    CONSOLE.println("Battery level fall to poweroff threshold");
#endif

  UPS.print("S");
  ups_cmd_sent = true;
  ups_shutdown = true;
  ups_go_2_shutdown = true; 
  
  if ( standalone == 0 ) {
    send_alarm_ab_shutdown( status );
  }
}
