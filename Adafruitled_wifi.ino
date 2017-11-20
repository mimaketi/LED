#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <iterator>
#include <vector>
#include "FastLED.h"
#ifdef __AVR__
  #include <avr/power.h>
#endif
char wssid[20];
char wpass[20];
String ssid = "";
String password = "";
String configureHTML = "";
String mainHTML;
String NAME = "unconfigured LEDESP";
std::vector<char*> handlers;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#define DATA_PIN    2
uint16_t COLOR_ORDER = NEO_RGB;
int NUM_LEDS = 98;
//BedBack: 148
//light hall: 117
//lamp: 92

//######EEPROM######
int eessid = 0;
int eepass = 20;
int eenumLED = 40;
int eename = 60;
int eegrb = 80;
//######EEPROM######

float ledDelay = 1000/120;
int delayF1 = 120;
int brightness = 255;
int currLED = 0;
int startPos = 0;
int i = 0;
long long r = 0;
long long g = 0;
long long b = 0;
uint8_t u8R = 0;
uint8_t u8G = 0;
uint8_t u8B = 0;
String rgb = "#ffffff";
long lngRGB = 0;
bool rev = false;
long unsigned int milli = 0;
long long number = 0;
String temp = "";
String lastCall = "clear";

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

/////

Adafruit_NeoPixel strip = Adafruit_NeoPixel();
CRGBPalette16 gPal;

typedef enum State{  // <-- the use of typedef is optional
  scolorWipe,
  srainbow,
  stheaterChase,
  stheaterChaseRainbow,
  snone,
  srainbowWithGlitter, 
  sconfetti, 
  ssinelon, 
  sjuggle, 
  sbpm,
  smotion,
  sfire,
  srainbowFill,
  slight
};

State state = snone;




void ledDel(int delayF){
  ledDelay = 1000/delayF;
  delayF1 = delayF;
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  EEPROM.begin(512);
  EEPROM.get(eessid,ssid);
  EEPROM.get(eepass,password);
  EEPROM.get(eegrb,COLOR_ORDER);
  EEPROM.get(eenumLED,NUM_LEDS);
  EEPROM.get(eename,NAME);
  strip.updateType(COLOR_ORDER + NEO_KHZ800);
  strip.updateLength(NUM_LEDS);
  strip.setPin(DATA_PIN);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  ssid.toCharArray(wssid,ssid.length());
  password.toCharArray(wpass,password.length());
  WiFi.begin(wssid, wpass);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (MDNS.begin("esp8266")) {
  }

  server.on("/colorWipe", icolorWipe);
  handlers.push_back("colorWipe");
  server.on("/rainbow", irainbow);
  handlers.push_back("rainbow");
  server.on("/theaterChase", itheaterChase);
  handlers.push_back("theaterChase");
  server.on("/theaterChaseRainbow", itheaterChaseRainbow);
  handlers.push_back("theaterChaseRainbow");
  server.on("/setPixels", isetPixels);
  handlers.push_back("setPixels");
  server.on("/clear", iclear);
  handlers.push_back("clear");
  server.on("/rainbowWithGlitter", irainbowWithGlitter);
  handlers.push_back("rainbowWithGlitter");
  server.on("/confetti", iconfetti);
  handlers.push_back("confetti");
  server.on("/sinelon",isinelon);
  handlers.push_back("sinelon");
  server.on("/juggle", ijuggle);
  handlers.push_back("juggle");
  server.on("/fire", ifire);
  handlers.push_back("fire");
  server.on("/rainbowFill", irainbowFill);
  handlers.push_back("rainbowFill");
  server.on("/light", ilight);
  handlers.push_back("light");
  server.on("/setPixels", setPixels);
  server.on("/eeprom", ieeprom);
  server.on("/reset", ireset);
  server.onNotFound(handleNotFound);
  for (i=0;i<handlers.size();i++){
    temp += String("<input name=\"e\" type=\"radio\" value=\"./");
    temp += String(handlers[i]);
    temp += String("\"/><label >");
    temp += String(handlers[i]);
    temp += String("</label><br/>");
  }
  
  
  server.on("/", [](){
    mainHTML = "";
    mainHTML += String("<html><body><SCRIPT language=\"JavaScript\">function b(){document.a.action=document.querySelector('input[name=\"e\"]:checked').value; return true;}</script><div><h1><a>Welcome</a></h1><form name=\"a\" method=\"get\" onsubmit=\"return b();\"><div><p>Select an effect. Optionally add parameters below</p></div><ul ><li > <label >Effects </label><br/></li>");
    mainHTML += String(temp.substring(0,temp.indexOf(lastCall)+1+lastCall.length()) + " checked=\"checked\" " + temp.substring(temp.indexOf(lastCall)+1+lastCall.length()) );
    mainHTML += String("</li><h3>Parameters</h3><p></p><li ><label >Delay </label><div><input name=\"delay\" type=\"text\" maxlength=\"3\" value=\"");
    mainHTML += String(delayF1);
    mainHTML += String("\"/> </div><p ><small>1=slow | 1000=fastest | 150=default</small></p></li><li ><label >Brightness </label><div><input name=\"brightness\" type=\"text\" maxlength=\"3\" value=\"");
    mainHTML += String(brightness);
    mainHTML += String("\"/> </div><p ><small>1-255</small></p></li><li ><label >Color </label><div><input name=\"color\" type=\"color\" value=\"");
    mainHTML += String(rgb);
    mainHTML += String("\"/> </div></li><li > <input type=\"submit\" value=\"Submit\"/></li></ul></form></div></body></html>");
    
    server.send(200, "text/html", mainHTML); 
  });
  server.on("/configure", [](){ 
    configureHTML = "";
    configureHTML.concat("<html><head></head><body><div><h1><a>Configuration</a></h1><form method=\"get\" action=\"./eeprom\"><div ><h2>LEDConfig</h2><p>This allows you to edit the base setting of your LED strip.</p></div><ul ><li id=\"li_1\" ><label>SSID</label><div><input name=\"0\" type=\"text\" maxlength=\"255\" value=\"");
    configureHTML.concat(wssid);
    configureHTML.concat("\"/> </div></li><li><label >Password</label><div><input name=\"20\" type=\"text\" maxlength=\"255\" value=\"");
    configureHTML.concat(wpass);
    configureHTML.concat("\"/> </div></li><li><label >Number Of LED's</label><div><input name=\"40\" type=\"text\" maxlength=\"255\" value=\"");
    configureHTML.concat(NUM_LEDS);
    configureHTML.concat("\"/> </div></li><li><label >Name</label><div><input name=\"60\" type=\"text\" maxlength=\"255\" value=\"");
    configureHTML.concat(NAME);
    configureHTML.concat("\"/> </div></li><li ><label >Color Order</label><div><select name=\"80\"><option value=\"rgb\"  selected=\"selected\" >rgb</option><option value=\"rbg\" >rbg</option><option value=\"grb\" >grb</option><option value=\"gbr\" >gbr</option><option value=\"brg\" >brg</option><option value=\"bgr\" >bgr</option></select></div></li><li class=\"buttons\"> <input type=\"hidden\"/> <input type=\"submit\" name=\"\" value=\"Submit\"/></li></ul></form></div></body></html>");

    server.send(200, "text/html", configureHTML);
  });

  
  gPal = HeatColors_p;

  

  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  
}

void redirectHome(){
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}

void loop() {
  server.handleClient();
   // Call the current pattern function once, updating the 'leds' array
  switch(state){
    case srainbow:
      rainbow();break;
    case scolorWipe:
      colorWipe();break;      
    case stheaterChase:
      theaterChase();break;
    case stheaterChaseRainbow:
      theaterChaseRainbow();break;
    case snone:
      delay(50);break;
    case srainbowWithGlitter: 
      rainbowWithGlitter();break;
    case sconfetti:
      confetti();break;
    case ssinelon:
      sinelon();break;
    case sjuggle:
      juggle();break;
    case sfire:
      fire();break;
    case srainbowFill:
      rainbowFill();break;
    case slight:
      light();break;
    
  }
  strip.show();
  FastLED.delay(ledDelay);
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  if (gHue > 255){gHue = 0;}
}

void ieeprom(){ 
  for (i=0; i<server.args(); i++){
    switch(server.argName(i).toInt()){
      case 0:
        EEPROM.put(eessid,server.arg(i));
        break;
      case 20:
        EEPROM.put(eepass,server.arg(i));
        break;
      case 80:
        if (server.arg(i) == "rgb"){COLOR_ORDER = NEO_RGB;}
        if (server.arg(i) == "rbg"){COLOR_ORDER = NEO_RBG;}
        if (server.arg(i) == "grb"){COLOR_ORDER = NEO_GRB;}
        if (server.arg(i) == "gbr"){COLOR_ORDER = NEO_GBR;}
        if (server.arg(i) == "brg"){COLOR_ORDER = NEO_BRG;}
        if (server.arg(i) == "bgr"){COLOR_ORDER = NEO_BGR;}
        EEPROM.put(eegrb,COLOR_ORDER);
        strip.updateType(COLOR_ORDER + NEO_KHZ800);
        break;
      case 40:
        EEPROM.put(eenumLED,server.arg(i).toInt()); 
        strip.updateLength(server.arg(i).toInt());
        break;
      case 60:
        EEPROM.put(eename,server.arg(i));
        break;
    }
    EEPROM.commit();
    redirectHome();
  }
}

void getArgs(int delayF){
  bool delSet = false;
  for (uint8_t i=0; i<server.args(); i++){
     if (server.argName(i) == "brightness"){
       //FastLED.setBrightness(server.arg(i).toInt());
       strip.setBrightness(server.arg(i).toInt());
       brightness = server.arg(i).toInt();
     }
     if (server.argName(i) == "delay"){
       ledDel(server.arg(i).toInt());
       delSet = true;
     }
     if (server.argName(i) == "color"){
      number = strtol( server.arg(i).substring(1,server.arg(i).length()).c_str(), NULL, 16);
      rgb = String(server.arg(i));
      r = number >> 16;
      g = number >> 8 & 0xFF;
      b = number & 0xFF;
     }
  }
  if (!delSet){
    ledDel(delayF);
  }
  redirectHome();
}

void icolorWipe(){
  state = scolorWipe;
  lastCall = "colorWipe";
  getArgs(120);
};
void ilight(){
  state = slight;
  lastCall = "light";
  getArgs(120);
}
void irainbow(){
  state = srainbow;
  lastCall = "rainbow";
  getArgs(120);
};
void irainbowFill(){
  lastCall = "rainbowFill";
  state = srainbowFill;
  getArgs(120);
};
void itheaterChase(){
  lastCall = "theaterChase";
  state = stheaterChase;
  getArgs(5);
};
void itheaterChaseRainbow(){
  lastCall = "theaterChaseRainbow";
  state = stheaterChaseRainbow;
  getArgs(5);
};
void isetPixels(){
  lastCall = "setPixels";
  state = snone;
  redirectHome();
  setPixels();
};
void iclear(){
  lastCall = "clear";
  state = snone;
  redirectHome();
  clearPixels();
};
void irainbowWithGlitter(){
  lastCall = "rainbowWithGlitter";
  state = srainbowWithGlitter;
  getArgs(120);
};
void iconfetti(){
  lastCall = "Confetti";
  state = sconfetti;
  getArgs(120);
};
void isinelon(){
  lastCall = "sinelon";
  state = ssinelon;
  getArgs(120);
};
void ijuggle(){
  lastCall = "juggle";
  state = sjuggle;
  getArgs(120);
};
void ifire(){
  lastCall = "fire";
  //random16_add_entropy( random(millis));
  state = sfire;
  getArgs(60);
}
void ireset(){
  ESP.reset();
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void fadeToBlack(int amt){
  for (i = 0; i<NUM_LEDS;i++){
     lngRGB = strip.getPixelColor(i);
     u8R = (uint8_t)((lngRGB >> 16) );
     u8G = (uint8_t)((lngRGB >> 8) );
     u8B = (uint8_t)(lngRGB );
     if (u8R < amt){u8R = amt;}
     if (u8G < amt){u8G = amt;}
     if (u8B < amt){u8B = amt;}
     strip.setPixelColor(i, u8R - amt, u8G - amt, u8B - amt);
  }
}

void rainbow(){
  for (i = 0; i<NUM_LEDS;i++){
    strip.setPixelColor(i, Wheel(gHue+i));
  }
}

void light(){
  for (i = 0; i<NUM_LEDS;i++){
    strip.setPixelColor(i, strip.Color(r,g,b));
  }
  state = snone;
}

void rainbowFill(){
   for (i = 0; i<NUM_LEDS;i++){
    strip.setPixelColor(i, Wheel(gHue));
  }
}

void rainbowWithGlitter(){
  rainbow();
  addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter){
   if(random8() < chanceOfGlitter){
     strip.setPixelColor(random16(NUM_LEDS),strip.Color(255,255,255)); 
   }
}

void confetti(){
  fadeToBlack(5);
  strip.setPixelColor(random16(NUM_LEDS), Wheel(gHue + random8(64)));
}

void sinelon(){
  fadeToBlack(10);
  strip.setPixelColor(beatsin16( 13, 0, NUM_LEDS-1 ), Wheel(gHue));
  
}

void juggle(){
  fadeToBlack(20);
  byte dothue = 1;
  for(i = 0; i < 8; i++) {
    lngRGB = strip.getPixelColor(i);
    strip.setPixelColor(beatsin16( i+7, 0, NUM_LEDS-1 ), lngRGB |= Wheel(dothue));
    dothue += 32;
  }
}

void colorWipe(){
  if (currLED >= NUM_LEDS){
    currLED = 0;
    rev = !rev;
  }
  if (!rev){
    strip.setPixelColor(currLED, strip.Color(r,g,b));
    currLED++;
  }else{
    strip.setPixelColor(currLED, strip.Color(0,0,0));
    currLED++;
  }
}

void theaterChase(){
  if (currLED >= 3){
    currLED = 0;
  }
  for (i = currLED; i<NUM_LEDS; i++){
    if ((i-currLED)%3 == 0){
      strip.setPixelColor(i,strip.Color(r,g,b));
      strip.setPixelColor(i-1,strip.Color(0,0,0));
    }
  }
  currLED++;
}

void theaterChaseRainbow(){
    if (currLED >= 3){
    currLED = 0;
  }
  if (startPos >= NUM_LEDS){
    startPos = 0;
  }
  for (i = currLED; i<NUM_LEDS; i++){
    if ((i-currLED)%3 == 0){
      strip.setPixelColor(i,Wheel(i));
      strip.setPixelColor(i-1,strip.Color(0,0,0));    
    }
  }
  currLED++;
  startPos += 15;
}

void clearPixels(){
  strip.clear();
}

void setPixels(){
  
}




#define COOLING 50
#define SPARKING 100
void fire()
{
// Array of temperature readings at each simulation cell
   static bool test = true;
   static byte *heat;
   if (test){
    heat = (byte*)malloc(NUM_LEDS * sizeof(byte));
    test = false;
   }
  // Step 1.  Cool down every cell a little
    for( i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }
    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( 0 ) { //reverse
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      strip.setPixelColor(pixelnumber, color.r,color.g,color.b);
    }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
uint8_t Wheelr(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return 255 - WheelPos * 3;
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return 0;
  }
  WheelPos -= 170;
  return WheelPos * 3;
}
uint8_t Wheelg(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return 0;
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return WheelPos * 3;
  }
  WheelPos -= 170;
  return 255 - WheelPos * 3;
}
uint8_t Wheelb(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return WheelPos * 3;
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return 255 - WheelPos * 3;
  }
  WheelPos -= 170;
  return 0;
}
