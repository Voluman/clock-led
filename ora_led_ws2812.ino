/* 
 * RDS Led óra 2020.12.02.
 */
 
#include <FastLED.h>
#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_MCP9808.h"
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <UnixTime.h>

RTC_DS1307 rtc;
#define LED_PIN  9
#define NUM_LEDS 60
CRGB leds[NUM_LEDS];
int LDR_Pin = A3; // fénymérő
int markesz = 0;
// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
UnixTime stamp(2); // időzóna eltolódás

void setup() {  
  Ethernet.init(10);  // Most Arduino shields
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);  
  while (!Serial); // for Leonardo/Micro/Zero
  Serial.begin(9600);
  // start Ethernet and UDP
  if ( Ethernet.begin( mac ) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  Udp.begin(localPort);  
  if (! rtc.begin()) {
    Serial.println("Nincs meg az RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC nem működik!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime( 2020, 12, 2, 19, 35, 20));
  }  
  
  if (!tempsensor.begin(0x18)) {
    Serial.println("Couldn't find MCP9808! Check your connections and verify the address is correct.");
    while (1);
  }  
  tempsensor.setResolution(2); // sets the resolution 2 0.125°C 130 ms
}

void loop() {
  // idő beolvasás  
  DateTime now = rtc.now();     
  while (Serial.available() > 0 ) {
    String str = Serial.readString();
    if (str.substring(0,2) =="20") {
    rtc.adjust(DateTime((word)(2000 + (str.substring( 2, 4).toInt())) , 
                        (byte)str.substring( 4, 6).toInt(), 
                        (byte)str.substring( 6, 8).toInt(),
                        (byte)str.substring( 8, 10).toInt(),
                        (byte)str.substring(10,12).toInt(),
                        (byte)str.substring(12,14).toInt()));
      Serial.println();
      Serial.print( "Beállítva: " + str.substring(0,4) + "." + str.substring(4,6) + "." + str.substring(6,8) + " " +
                  str.substring(8,10) + ":" + str.substring(10,12) + ":" + str.substring(12,14)  );

      Serial.println();
    }
  }
  orakiir() ;
}

void orakiir() {
  tempsensor.wake();  // wake up, ready to read!
  float c = tempsensor.readTempC(); 
  int fenyny = analogRead( LDR_Pin ); 
  // idő beolvasás  
  DateTime now = rtc.now();     
  int brightness = 32; // Initial brightness 255
  brightness = floor( fenyny / 10 );  
  if ( brightness < 2 ) brightness = 2;
  if ( brightness > 255 ) brightness = 255;
  int perc = now.minute();
  int tperc = 0;
  int ora = now.hour();
  int tora = 0;
  int mp = now.second();    
  // sietés visszaállítása
  if ( markesz == 0 ) {
    if ( ( perc == 1 ) and ( mp == 30 ) ) { // ( ora == 3 ) and 
      //rtc.adjust(DateTime( now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()-6)); 
      sendNTPpacket(timeServer);
      delay(1000);
      if (Udp.parsePacket()) {
        Udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;  
        stamp.getDateTime(epoch);
        rtc.adjust(DateTime( stamp.year, stamp.month, stamp.day, 
                             stamp.hour, stamp.minute, stamp.second)); 
        Ethernet.maintain();
        markesz = 1;
      }
    }
  } else {
    if ( perc > 5 ) { // ( ora >= 3 ) and (  )
      markesz = 0;
    }
  }  
  // kijelzó színének változtatása órák szerint
  int p = 106; int z = 90; int k = 205; // sky blue, SlateBlue
  if ( ora > 18 ) { p = 255;  z = 165;  k = 0; } // narancs 
  if ( ora > 20 ) { p = 255;  z = 0;  k = 0; } // piros 
  if ( ora > 22 ) { p = 200;  z = 0;  k = 0; } // piros     
  if ( ( ora >= 0 ) and ( ora < 4 ) ) {  p = 150;  z = 0; k = 0; } // piros
  //if ( ( ora > 3 ) and ( ora < 7 ) ) { p = 16;  z = 0;  k = 128; } // lila 
  if ( ( ora > 3 ) and ( ora < 6 ) ) { p = 0;  z = 0;  k = 201;  } // blue
  if ( ( ora > 5 ) and ( ora < 8 ) ) { p = 0;  z = 0;  k = 255;  } // blue
  if ( ( ora > 10 ) and ( ora < 14 ) ) {  p = 255;  z = 255;  k = 255; } // fehér
  if ( ( mp > 45 ) and ( mp < 47 ) ){
    // hőmérséklet kiírás
    leds[28] = CRGB (0, 0, 0);
    leds[29] = CRGB (0, 0, 0);
    leds[30] = CRGB ( p, z, k  );
    leds[45] = CRGB ( p, z, k  );   
    int ho1 = 0; //2021
    int ho2 = 0; 
    int ho3 = 0;
    if ( c >= 30 ) { ho1 = 3; ho2 = floor( (c - 30) ); ho3 = floor((c - floor(c/10)) * 10 ); } 
    if (( c >= 20 ) and ( c < 30 )) { ho1 = 2; ho2 = floor( (c - 20) ); ho3 = floor((c - floor(c)) * 10 ); } 
    if (( c >= 10 ) and ( c < 20 )) { ho1 = 1; ho2 = floor( (c - 10) ); ho3 = floor((c - floor(c)) * 10 ); } 
    if (( c >= 0 ) and ( c < 10 )) { ho1 = 0; ho2 = floor( c  ); ho3 = floor((c - floor(c)) * 10 ); } 
    Serial.print(c, 1);
    Serial.print("*C");  
    Serial.println();    
    szamkiir( 0, ho1,   p, z, k );
    szamkiir( 14, ho2,   p, z, k );
    szamkiir( 31, ho3, p, z, k );
    szamkiir( 46, 11,  p, z, k ); 
   FastLED.setBrightness(brightness);  
    FastLED.show(); 
    delay(5000);    
  } else {
    tora = floor( ora / 10 );
    ora = ora - ( tora * 10 );
    tperc = floor( perc / 10 );
    perc = perc - ( tperc * 10 );    
    szamkiir( 0, tora,   p, z, k );
    szamkiir( 14, ora,   p, z, k );
    szamkiir( 31, tperc, p, z, k );
    szamkiir( 46, perc,  p, z, k );     
    if ( ( ( now.second() % 2 ) == 0) or ( ( now.hour() > 21 ) or ( now.hour() < 6) ) ) { 
      leds[28] = CRGB ( p, z, k );
      leds[29] = CRGB ( p, z, k );
    } else { 
      leds[28] = CRGB (0, 0, 0);
      leds[29] = CRGB (0, 0, 0);
    }
    leds[30] = CRGB ( 0,0,0 );
    leds[45] = CRGB ( 0,0,0 );   
    FastLED.setBrightness(brightness);  
    FastLED.show(); 
  }
  tempsensor.shutdown_wake(1); // hőmérő kikapcsolása  
}
 
void szamkiir(int kezdo, int szam, int r, int g, int b ) {
  leds[kezdo + 0 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 1 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 2 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 3 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 4 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 5 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 6 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 7 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 8 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo + 9 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo +10 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo +11 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo +12 ] = CRGB ( 0, 0, 0 ); 
  leds[kezdo +13 ] = CRGB ( 0, 0, 0 );

  if ( ( ( szam > 3 ) and ( szam != 7 ) ) or ( szam < 1 ) ) {
    leds[kezdo + 0 ] = CRGB ( r, g, b ); 
    leds[kezdo + 1 ] = CRGB ( r, g, b ); 
  }
  if ( ( szam != 1 ) and ( szam != 4 ) ) {
    leds[kezdo + 2 ] = CRGB ( r, g, b ); 
    leds[kezdo + 3 ] = CRGB ( r, g, b ); 
  }
  if ( ( szam != 5 ) and ( szam != 6 ) and ( szam != 11) ) {
    leds[kezdo + 4 ] = CRGB ( r, g, b ); 
    leds[kezdo + 5 ] = CRGB ( r, g, b ); 
  }
  if ( ( szam != 1 ) and ( szam != 7 ) and ( szam != 0) and ( szam != 11 ) ) {
    leds[kezdo + 6 ] = CRGB ( r, g, b ); 
    leds[kezdo + 7 ] = CRGB ( r, g, b ); 
  }
  if ( ( szam == 2 ) or ( szam == 6 ) or ( szam == 8 ) or ( szam == 0 ) or ( szam == 11 ) ) {
    leds[kezdo + 8 ] = CRGB ( r, g, b ); 
    leds[kezdo + 9 ] = CRGB ( r, g, b ); 
  } 
  if ( ( szam != 1 ) and ( szam != 4 ) and ( szam != 7 ) ) {
    leds[kezdo + 10 ] = CRGB ( r, g, b ); 
    leds[kezdo + 11 ] = CRGB ( r, g, b ); 
  } 
  if ( ( szam != 2 ) and ( szam != 11 ) ) {
    leds[kezdo + 12 ] = CRGB ( r, g, b ); 
    leds[kezdo + 13 ] = CRGB ( r, g, b ); 
  } 
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
