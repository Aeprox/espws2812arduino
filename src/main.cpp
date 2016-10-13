#include "main.h"

// Update these with values suitable for your network.
const char* ssid = "Parnonetv6";
const char* password = "86kxbv56";
const char* mqtt_server = "192.168.4.145";
const int mqqt_port = 1883;

CRGB leds[NUM_LEDS];
byte ledBrightness = 96;
byte currentFx = NULL;
byte fxCounter = 0;
long lastFx = 0;
int fxDelay = 25;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
long keepalivetime = 2*60*1000;

void setup() {
  Serial.begin(115200);

  setup_wifi();

  //setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //setup FastLED
  FastLED.addLeds<CHIPSET, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);

  // setup OTA
  ArduinoOTA.onStart([]() {
      Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char str[length+1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    str[i] = (char) payload[i];
  }
  str[length] = '\0';

  if(!strcmp(topic,"ledstrip/0/cmd/solid")){
    disableFx();

    char r[4] = {'\0','\0','\0','\0'},g[4]= {'\0','\0','\0','\0'},b[4]= {'\0','\0','\0','\0'};
    char * start,* i2,* i3, * fin;
    start = strchr(str,'(');
    i2 = strchr(str,',');
    i3 = strrchr(str,',');
    fin = strrchr(str,')');

    strncpy(r,++start,(int)i2-(int)start);
    strncpy(g,++i2,(int)i3-(int)i2);
    strncpy(b,++i3,(int)fin-(int)i3);

    byte ri = atoi(r);
    byte gi = atoi(g);
    byte bi = atoi(b);
    fill_solid(leds, NUM_LEDS, CRGB(ri,gi,bi));
    FastLED.show();
  }
  else if (!strcmp(topic,"ledstrip/0/cmd/fx")){
    if(!strcmp(str,"rainbow")){
      currentFx = 1;
      Serial.println("Started rainbow fx");
    }
    else if (!strcmp(str,"chaser")){
      currentFx = 2;
      Serial.println("Started chaser");
    }
  }
  else if (strstr(topic,"ledstrip/0/cmd/delay")){
    fxDelay = atoi(str);
    Serial.println("Updated delay");
  }
  else if (strstr(topic,"ledstrip/0/cmd/brightness")){
    FastLED.setBrightness(atoi(str));
    FastLED.show();
    Serial.println("Updated brightness");
  }
  else{
    Serial.println("Couldnt parse payload, unknown topic");
     for (int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ledstrip/0/status", "I'm alive!");
      // ... and resubscribe
      client.subscribe("ledstrip/0/cmd/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void disableFx(){
  currentFx = NULL;
}
void runRainbowFx(){
  fill_rainbow( leds, NUM_LEDS, fxCounter, 7);
  FastLED.show();
  fxCounter++;
}
void runChaseFx(){
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( fxCounter, 255, 192);
  FastLED.show();
  fxCounter++;
}
void runFx(){
  switch (currentFx) {
    case 1:
      runRainbowFx();
      break;
    case 2:
      runChaseFx();
      break;
    default:
      break;
  }
}


void loop() {
  // handle MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // handle OTA
  ArduinoOTA.handle();

  //periodically deal with leds and send MQTT status report
  long now = millis();
  if(now - lastFx > fxDelay){
    lastFx = now;
    if(currentFx != NULL){
      runFx();
    }
  }
  if (now - lastMsg > keepalivetime) {
    lastMsg = now;
    snprintf (msg, 75, "Still here..(%d,%d)",currentFx,ledBrightness);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("ledstrip/0/status", msg);
  }
}
