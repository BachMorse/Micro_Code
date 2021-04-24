//effe testen
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>
#include <PubSubClient.h>
#include <WiFi.h>

LiquidCrystal_I2C lcd(0x27, 21, 4);
WiFiClient espClient;

#define SSID          "Sarah" ///*"NETGEAR68"*/ "DESKTOP-CEC32AM 8066"        //naam
#define PWD           "yoloxdxp" ///*"excitedtuba713"*/ "]16b571H"   //wachtwoord

#define MQTT_SERVER   "172.20.10.3"
#define MQTT_PORT     1883

PubSubClient client(espClient);

#define STIL 0
#define KORT 1
#define LANG 2

int langSignaal = 45;
int kortSignaal = 16;
int stilteSignaal = 3;

const int pin = 34;
const int led = 12;

//alle elementen op 0 zetten
// https://stackoverflow.com/questions/39909411/what-is-the-default-value-of-an-array-in-c
int volgorde[100] = {0};

int loper = 0;
int loper_morse = 0;
int loper_display = 0;
int lengte_morse = 0;

int lengteMorse = 44;
int morse[44] = {0};

int binnenkomend[44] = {0, 1, 0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1, 2, 0, 1, 0, 1, 0, 1, 2};
char binnenkomend_char[44] = {'0'};

int vorige_waarde = 0;
int som_alternatief = 0;

int pauze_afstand = 0;
int pauze_fitness = 0;

ezButton button(18);

/********************************
          COMMUNICATIE
          // callback function, only used when receiving messages
*********************************/

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  int messageTempArray[lengteMorse];

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // When receiving a message on "esp32/control" a check should be excecuted

  // If a message is received on the topic esp32/control, you check if the message is either "start" or "stop" (or "reset").
  // Changes the state according to the message
  if (String(topic) == "esp32/morse/control"){
    if(messageTemp.equals("0")){ //reset
      //niets eigenlijk
    }
    if(messageTemp.equals("1")){ //stop (afstand)
      pauze_afstand = 1;
    }
    if(messageTemp.equals("2")){ //start (ontsmetten)
      pauze_afstand = 0;
    }
    if(messageTemp.equals("3")){ //poweroff (stop fitness)
      pauze_fitness = 1;
    }
    if(messageTemp.equals("4")){ //poweron (start fitness)
      pauze_fitness = 0;
    }
  }
  else if (String(topic) = "esp32/morse/intern"){
    for(int i = 0; i<sizeof(messageTempArray); i++){
      binnenkomend_char[i] = messageTempArray[i];
    }
    //omzetten naar integer
    int volledig_int = atoi(binnenkomend_char);
    //integer omzetten naar array van integers
    for (int i = 44; i >= 0; i--) {
      binnenkomend[i] = volledig_int % 10;
      volledig_int /= 10;
    } 
  }
}

/********************************
            MQTT
*********************************/
void setup_wifi(){
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect(){
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Morse_Micro"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/********************************
          SETUP MORSE
*********************************/
void setup() {
  //pinMode(led, OUTPUT);
  Serial.begin(115200);
  pinMode(led, OUTPUT);

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}


/********************************
          MORSE CODE
*********************************/
int som(int rij[100]){
  int som = 0;
  for(int i=0; i<100; i++){
    som += rij[i];
  }
  return som;
/*
  //efficiënter via laatste aftrekken en nieuwste toevoegen
  som_alternatief = som_alternatief - vorige_waarde + volgorde[loper-1]; //f rij[...]
  Serial.print(som_alternatief);
  Serial.print(" \t");
  return som_alternatief;*/
}

void voegToe(int aV){
  //elke waarde toevoegen aan de array, tot 6, omdat het max signaal max 6 samples duurt
  if(loper < 100){
    vorige_waarde = volgorde[loper];
    volgorde[loper] = aV;
    loper++;
  }
  else if (loper == 100) {
    vorige_waarde = volgorde[loper];
    volgorde[99] = aV; 
    loper = 0;
  }
}

void voegToeMorse(int signaal){
  
  if (morse[loper_morse] != signaal){
    loper_morse++;
    morse[loper_morse] = signaal;
    Serial.print("\n");
    if(signaal == 0) {
      Serial.print("stil toegevoegd \n"); 
    }
    else if (signaal == 1) {
      Serial.print("kort toegevoegd \n");
      lcd.setCursor(loper_display,0);
      lcd.print(".");
      //Serial.print(".");
      Serial.print(loper_display);
      loper_display++;
    }
    else if(signaal == 2) {
      Serial.print("lang toegevoegd \n");
      lcd.setCursor(loper_display-1,0);
      //Serial.print("_");
      lcd.print("_");
      Serial.print(loper_display);
    }
  }
  if (signaal == LANG){
    //volgorde terug op 0 zetten, zodat geen kortsignaal meer gedetecteerd wordt
    for (int i = 0; i<100; i++){ //limiet zeker nakijken
      volgorde[i] = {0};
    }
    
  }

  if(loper_morse == (lengteMorse-1)){
    for (int i = 0; i<(lengteMorse); i++){
      Serial.print(morse[i]);
      Serial.print("\t");
    }
    loper_morse = 0;
  }
}

void vergelijk(){
  bool vergelijk_bool[lengteMorse] = {false};
  int teller = 0;
  for (int i = 0; i<lengteMorse; i++){
    if(morse[i] == binnenkomend[i]) {
      vergelijk_bool[i] = true;
      teller++;
    }
  }

  if (teller == lengteMorse ){
    lcd.setCursor(0,1);
    lcd.print("6");
    lcd.setCursor(0,2);
    lcd.print("Boekenrek");
    client.publish("esp32/morse/intern", "correcte code");
  } 
}



/********************************
          LOOP
          // callback function, only used when receiving messages
*********************************/
void loop() {

client.publish("esp32/morse/control", "correcte code");

//////////////////////////////////wifi connect
if (!client.connected()){
  reconnect();
}
 client.loop();

  button.loop();
  if(button.isPressed()){
    //volgorde = {100};
    for (int i = 0; i<lengteMorse; i++){
      morse[i] = 0;
      Serial.print(morse[i]);
      Serial.print(" \t");
      loper_morse = 0;
    }
    lcd.clear();
    loper_display = 0;
  }

  if(button.getStateRaw() == 0 && pauze_afstand == 0 && pauze_fitness == 0){ //button is active high
  //Serial.println("The button is pressed");
  
  //input lezen op pin X
  int analogValue = analogRead(pin);
  
  //output printen op computerscherm
  //Serial.print(analogValue);
  //Serial.print(" \t");


  //in array steken
  voegToe(analogValue);

  //som nemen om te vergelijken met een kort of een lang signaal en toevoegen wanneer het kort of lang is
  //een lang signaal wordt steeds vooraf gegaan door een kort signaal
  int sommetje = 0;
  sommetje = som(volgorde);
  /*Serial.print("som= ");
  Serial.print(sommetje);
  Serial.print(" \t");*/

  //wanneer de som van de array groter is dan hetgeen in de if-lus staat, 
  //dan zal in een nieuwe array worden genoteerd of er een kort of een lang signaal
  //werd gehoord
  //Na detectie van een lang signaal moeten alle waarden terug op 0 gezet worden
  //omdat er anders meerdere waarden lange signalen toegevoegd kunnen worden
  if (sommetje>4095*langSignaal){
    voegToeMorse(LANG);
    //Serial.print(" lang gedetecteerd \t");
    delay(1000);
  }

  else if (sommetje>4095*kortSignaal){
    voegToeMorse(KORT);
    //morse[loper_morse] = KORT;
    //loper_morse++;
    //Serial.print(" kort gedetecteerd \t"); 
  }
  else if (sommetje<4095*stilteSignaal){
    voegToeMorse(STIL);
    //Serial.print(" stil gedetecteerd \t");
  }
  vergelijk();

  //delay(20);
  delay(2);
  }

}