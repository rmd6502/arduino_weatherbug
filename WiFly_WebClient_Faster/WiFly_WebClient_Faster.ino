// (Based on Ethernet's WebClient Example)

#include "WiFly.h"

#include "Credentials.h"

Client client("i.wxbug.net", 80);
#define FMT "GET /REST/Direct/GetObs.ashx?la=%s&lo=%s&ic=1&api_key=%s HTTP/1.0"

//#define ALOHA

#ifdef ALOHA
#define LAT "21.2889"
#define LON "-157.8458"
#else
#define LAT "40.72534"
#define LON "-73.99708"
#endif

#define LBRACE '{'

#define DEBUG 

enum { 
  NOT_CONNECTED, CONNECTED, PARSING, TEMP, COLON, QUOTE, VALUE, VQUOTE
};

uint8_t current_state = NOT_CONNECTED;

void display_temp(double new_temp);
const int SDI = 2; //Red wire (not the red 5V wire!)
const int CKI = 3; //Green wire

void setup() {
  
  Serial.begin(115200);
  Serial.println("WebClient example at 9600 baud.");

pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);
  WiFly.begin();
  
  DEBUG("Associating");

  if (!WiFly.join(ssid, passphrase)) {
    Serial.println("Association failed.");
    while (1) {
      Serial.println("failed");
      // Hang on failure.
    }
  }  

  //WiFly.configure(WIFLY_BAUD, 38400);
}

char namebuf[32] = {0};
uint8_t nameptr = 0;

char tempbuf[32] = {0};
uint8_t tempptr = 0;

double current_temp = 0.0f;

void loop() {

  if (current_state == NOT_CONNECTED) {
      DEBUG("connecting...");
      if (client.connect()) {
        DEBUG("connected");
        char buf[128];
        sprintf(buf, FMT, LAT, LON, apiKey);
        client.println(buf);
        client.println("Host: i.wxbug.net");
        client.println();
        current_state = CONNECTED;
      } else {
        DEBUG("connection failed");
        current_state = NOT_CONNECTED;
        return;
      }
  }
  
  int ch = -1;  
  while (client.available()) {
    ch = client.read();
    //Serial.print((char)ch);
    switch (current_state) {
      case CONNECTED:
        if (ch == LBRACE) {
          current_state = PARSING;
          DEBUG("\nParsing\n");
        }
        break;
      case PARSING:
        if (isspace(ch)) break;
        if (ch == '"') {
          current_state = QUOTE;
          nameptr = 0;
          DEBUG("\nQuoted String\n");
        }
        break;
      case QUOTE:
        if (ch == '"') {
          namebuf[nameptr] = 0;
          current_state = COLON;
        } else {
          if (nameptr < sizeof(namebuf) - 1) {
            namebuf[nameptr++] = ch;
          }
        }
        break;
      case COLON:
        if (isspace(ch)) break;
        if (ch == ':') {
          if (!strcmp(namebuf, "temperature")) {
            current_state = TEMP;
            tempptr = 0;
            DEBUG("\n\nTemperature!\n\n");
          } else {
            current_state = VALUE;
            DEBUG("\n\nvalue\n\n");
          }
        } else {
          DEBUG("unknown char, ignoring");
        }
        break;
      case VALUE:
        switch (ch) {
          case ',':
            DEBUG("\n\nend value\n\n");
            current_state = PARSING;
            break;
          case '"':
            DEBUG("\n\nquote within value\n\n");
            current_state = VQUOTE;
            break;
        }
        break;
      case VQUOTE:
        if (ch == '"') {
          DEBUG("\n\nend value quote processing\n\n");
          current_state = VALUE;
        }
        break;
      case TEMP:
        if (tempptr < sizeof(tempbuf)-1 && (isdigit(ch) || ch == '.' || ch == '-')) {
          tempbuf[tempptr++] = ch;
        } else {
          tempbuf[tempptr] = 0;
          current_temp = strtod(tempbuf, NULL);
          Serial.print("temperature: ");
          Serial.println(current_temp);
          display_temp(current_temp);
          DEBUG("\n\nDone\n\n");
          client.stop();
          current_state = NOT_CONNECTED;
        }
    }
    if (!client.connected()) {
      DEBUG("disconnecting.");
      client.stop();
      current_state = NOT_CONNECTED;
    }
  }
}


const unsigned int STRIP_LENGTH = 32;
struct temp_rgb {
  double low_temp;
  uint8_t r,g,b;
} tempColors[] = {
  { 0, 0, 0, 64 },
  { 25, 0, 0, 128 },
  { 35, 128, 64, 200 },
  { 50, 128, 255, 100 },
  { 65, 160, 160, 32 },
  { 75, 255, 160, 32 },
  { 85, 255, 64, 32 },
  { 95, 255, 16, 16 },
  { 110, 255, 0, 0 }
};
void display_temp(double new_temp) {
  double displength = new_temp * STRIP_LENGTH / 100;
  if (displength < 0) displength = 0;
  if (displength > STRIP_LENGTH) displength = STRIP_LENGTH;
  Serial.print("displength "); Serial.println(displength);
  uint32_t dispcolor = 0;
  int c = 0;
  for (c = 0; c < sizeof(tempColors)/sizeof(tempColors[0]); ++c) {
    if (new_temp <= tempColors[c].low_temp) {
      Serial.print("have temp "); Serial.println(c);
      break;
    }
  }
  if (c > 0) --c;
  dispcolor = ((uint32_t)tempColors[c].r << 16L) + ((uint32_t)tempColors[c].g << 8L) + ((uint32_t)tempColors[c].b);
  Serial.print("dispcolor "); Serial.println(dispcolor);
  double r = displength - floor(displength);
  uint32_t dimcolor = ((uint32_t)(tempColors[c].r * r) << 16) | ((uint32_t)(tempColors[c].g * r) << 8) | ((uint32_t)(tempColors[c].b * r));
      Serial.print("dim dispcolor "); Serial.println(dispcolor);
  
  for(int LED_number = 0 ; LED_number < STRIP_LENGTH ; LED_number++) {
    if (LED_number == (int)displength + 1) {
      dispcolor = dimcolor;
    } else if (LED_number > displength) {
      dispcolor = 0;
    }
    
//The 1'L' forces the 1 to start as a 32 bit number, otherwise it defaults to 16-bit.    
    long mask = 1L << 23L;
    for( ; mask != 0 ; mask >>= 1L) {
      //Feed color bit 23 first (red data MSB)
      
      digitalWrite(CKI, LOW); //Only change data when clock is low
      
      if(dispcolor & mask) 
        digitalWrite(SDI, HIGH);
      else
        digitalWrite(SDI, LOW);
      
      digitalWrite(CKI, HIGH); //Data is latched when clock goes high
    }
    //Pull clock low to put strip into reset/post mode
    digitalWrite(CKI, LOW);
    delayMicroseconds(500); //Wait for 500us to go into reset
  }
}
