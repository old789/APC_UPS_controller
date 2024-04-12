
void eeprom_save(){
  EEPROM.put(0, mark);
  EEPROM.put(PT_SSID, ssid);
  EEPROM.put(PT_PASSW, passw);
  EEPROM.put(PT_HOST, host);
  EEPROM.put(PT_PORT, port);
  EEPROM.put(PT_URI, uri);
  EEPROM.put(PT_CRC, ram_crc());
  EEPROM.commit();
}

unsigned long ram_crc() {
  unsigned long crc = ~0L;
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
  byte *buf = (byte*)malloc( SIZE_EEPROM + 8 );

  if ( ! buf ){
    if ( enable_cli )
      Serial.println("Can't allocate memory");
    return(0);
  }

  memset(buf, 0, SIZE_EEPROM + 8);
  memcpy(buf, &mark, sizeof(mark));
  memcpy(buf+PT_SSID, &ssid, strlen(ssid));
  memcpy(buf+PT_PASSW, &passw, strlen(passw));
  memcpy(buf+PT_HOST, &host, strlen(host));
  memcpy(buf+PT_PORT, &port, sizeof(port));
  memcpy(buf+PT_URI, &uri, strlen(uri));

  for (uint16_t index = 0 ; index <= SIZE_EEPROM  ; ++index) {
    crc = crc_table[(crc ^ buf[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (buf[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  free(buf);
  return crc;
}

bool eeprom_read(){
uint16_t m = 0;
unsigned long crc = 0;

  EEPROM.get(0, m);
  if ( m != mark ) {
    if ( enable_cli ) {
      Serial.print("EEPROM read ERROR: incorrect marker, need ");Serial.print(mark,HEX);Serial.print("H but read ");Serial.print(m,HEX);Serial.println("H");
    }
    return(false);
  }

  EEPROM.get(PT_SSID, ssid);
  EEPROM.get(PT_PASSW, passw);
  EEPROM.get(PT_HOST, host);
  EEPROM.get(PT_PORT, port);
  EEPROM.get(PT_URI, uri);
  EEPROM.get(PT_CRC, crc);

  if ( crc != ram_crc() ){
    if ( enable_cli ) {
      Serial.println("EEPROM read ERROR: incorrect crc");
    }
    return(false);
  }
  return(true);
}

bool is_conf_correct(){
  if ( ( input_tries == 0 ) || ( strlen(ssid) == 0 ) || ( strlen(passw) == 0 ) || ( strlen(host) == 0 ) || ( port == 0 ) || ( strlen(uri) == 0 ) ){
    return(false);
  }
  return(true);
}
