//test
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

LiquidCrystal_I2C lcd(0x27, 21, 4);
WiFiClient espClient;

#define SSID          "OnePlus jeff" // "NETGEAR68" //"DESKTOP-CEC32AM 8066"        //naam
#define PWD            "jeffhotspot" // "excitedtuba713" //"]16b571H"   //wachtwoord

#define MQTT_SERVER   "broker.mqttdashboard.com" //"192.168.1.2"
#define MQTT_PORT     1883

PubSubClient client(espClient);

#define STIL 0
#define KORT 1
#define LANG 2

int test = 0;

int langSignaal = 45;
int kortSignaal = 10;
int stilteSignaal = 3;

const int pin = 34;
const int led = 12;

//alle elementen op 0 zetten
int volgorde[100] = {0};

//aanduidingen van plaats in array & locatie
int loper = 0;
int loper_morse = 0;
int loper_display = 0;
int lengte_morse = 0;
int loper_binnenkomend = 0;

int lengteMorse = 44;
int morse[44] = {0};

int binnenkomend[44] = {0, 1, 0, 1};
char binnenkomend_char[44] = {'0'};

int eerste_keer = 1;
int random_alohomora = 0;

int vorige_waarde = 0;
//int som_alternatief = 0;

int pauze_afstand = 0;
int pauze_fitness = 0;

int start = 1;
int einde = 0;

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

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  //kanalen main broker
  if (String(topic) == "esp32/morse/control"){
    if(messageTemp.equals("0")){ //reset
      //volledig opnieuw opstarten
      WiFi.disconnect();
      delay(10);
      ESP.restart();
    }
    if(messageTemp.equals("1")){ //stop (afstand)
      pauze_afstand = 1;
      lcd.clear();
      lcd.noBacklight();
      for (int i = 0; i<lengteMorse; i++){
      morse[i] = 0;
      loper_morse = 0;
      loper_display = 0;
    }
    }
    if(messageTemp.equals("2")){ //start (ontsmetten)
      pauze_afstand = 0;
      lcd.backlight();
    }
    if(messageTemp.equals("3")){ //poweroff (stop fitness)
      pauze_fitness = 1;
      lcd.clear();
      lcd.noBacklight();
      for (int i = 0; i<lengteMorse; i++){
      morse[i] = 0;
      loper_morse = 0;
      loper_display = 0;
    }
    }
    if(messageTemp.equals("4")){ //poweron (start fitness)
      pauze_fitness = 0; 
      lcd.backlight();    
    }
  }

  //kanalen van en naar speaker
  else if (String(topic) == "esp32/morse/intern"){
    binnenkomend[loper_binnenkomend] = atoi(messageTemp.c_str());
    loper_binnenkomend++;
    
    for (int i = 0; i<44; i++){
      Serial.print(binnenkomend[i]);
    }
  }

  else if(String(topic) == "esp32/morse/speaker_end"){
    start = 0;
  }

  //kanalen van telefoon
  else if (String(topic) == "esp32/fitness/telefoon"){
    if(messageTemp.equals("BEL")){
      lcd.backlight();
    }
  }
}

/********************************
        MQTT & WiFi
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
    if (client.connect("Morse_micro"))
    {
      Serial.println("connected");
      // Subscriben op kanalen
      client.subscribe("esp32/morse/control");
      client.subscribe ("esp32/morse/intern");
      client.subscribe ("esp32/morse/speaker_end");
      client.subscribe("esp32/fitness/telefoon");
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

  // tijd instellen hoe lang niet mag gereageerd worden op 
  // inkomend signaal nadat de button werd ingedrukt
  button.setDebounceTime(100);

  // initialize LCD
  lcd.init();

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}


/********************************
          MORSE
*********************************/
int som(int rij[100]){
  // telkens opnieuw de som nemen van array
  int som = 0;
  for(int i=0; i<100; i++){
    som += rij[i];
  }
  return som;
}

void voegToe(int aV){
  // elke waarde toevoegen aan de array
  if(loper < 100){
    vorige_waarde = volgorde[loper];
    volgorde[loper] = aV;
    loper++;
  }
  // vanaf begin opnieuw toevoegen wanneer einde bereikt is
  else if (loper == 100) {
    vorige_waarde = volgorde[loper];
    volgorde[99] = aV; 
    loper = 0;
  }
}

void voegToeMorse(int signaal){
  // voorwaarde: niet 2 keer na elkaar toevoegen
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
      loper_display++;
    }
    else if(signaal == 2) {
      Serial.print("lang toegevoegd \n");
      lcd.setCursor(loper_display-1,0);
      lcd.print("_");
    }
  }
  if (signaal == LANG){
    //volgorde terug op 0 zetten, zodat geen kortsignaal meer gedetecteerd wordt
    for (int i = 0; i<100; i++){ 
      volgorde[i] = {0};
    }
  }

  //volgende lijnen enkel laten staan wanneer lengteMorse
  //bereikt wordt met loper_morse
  if(loper_morse == (lengteMorse-1)){
    for (int i = 0; i<(lengteMorse); i++){
      Serial.print(morse[i]);
      Serial.print("\t");
    }
    loper_morse = 0;
  }
}

// visueel foutsignaal geven als er 1 keer verkeerd wordt gefloten
void vergelijk_fout(){
  for (int i = 0; i<=loper_morse; i++){
    if(morse[i] != binnenkomend[i]) {
      lcd.setCursor(0,1);
      lcd.print("Fout");
    }
  }
}

//vergelijken of gefloten overeenkomt met binnenkomend
void vergelijk(){
  int teller_vergelijk = 0;
  for (int i = 0; i<lengteMorse; i++){
    if(morse[i] == binnenkomend[i]) {
      // elke keer wanneer ze overeenkomen, tellen we 1 bij op
      teller_vergelijk++;
    }
    
  }

  //wanneer de teller gelijk is aan de lengte van de morse sequentie
  //wil dit zeggen dat elk element overeenkomt
  //en dus is de puzzel tot een goed einde gebracht
  if (teller_vergelijk == lengteMorse ){
    if(eerste_keer == 1){
      srand(time(0)); //om elke keer opnieuw een ander getal te krijgen
      random_alohomora = rand() % 10; //random getal tussen 0 en 10
      eerste_keer = 0;
      Serial.print("\n");
      Serial.print("random getal: ");
      Serial.print(random_alohomora);
      client.publish("esp32/morse/intern", "correct"); //speaker stopt met morse
      client.publish("esp32/morse/output", "einde_morse"); //5G-puzzel kan geactiveerd worden
      String random_string_alohomora = (String)(random_alohomora);
      const char *random_char_alohomora = random_string_alohomora.c_str(); //integer omzetten naar een karakter
      //of via itoa
      client.publish("esp32/alohomora/code2", random_char_alohomora); //random nummer wordt doorgestuurd naar alohomora
      einde = 1;
    }
    lcd.setCursor(0,1);
    lcd.print(random_alohomora);
    lcd.setCursor(0,2);
    lcd.print("VUILBAK"); //tip waar afstandsbediening ligt
  }
}


/********************************
          MORSE LOOP
*********************************/
void loop() {

//////////////////////////////////wifi connect
if (!client.connected()){
  reconnect(); //reconnecten wanneer nog niet gebeurd OF wanneer is weggevallen
}
 client.loop();
  button.loop();
  if(button.isPressed() && einde == 0){ //enkel reageren op button wanneer spel nog niet is afgelopen
    for (int i = 0; i<lengteMorse; i++){
      morse[i] = 0;
      loper_morse = 0;
    }
    for (int i = 0; i<100; i++){ //limiet zeker nakijken
      volgorde[i] = {0};
    }
    lcd.clear();
    loper_display = 0;
  }

  if(button.getStateRaw() == 0 && pauze_afstand == 0 && pauze_fitness == 0 && einde == 0 && start == 0){ //button is active high

    //input lezen op pin
    int analogValue = analogRead(pin);

    //in array steken
    voegToe(analogValue);

    //som nemen om te vergelijken met een kort of een lang signaal en toevoegen wanneer het kort of lang is
    //een lang signaal wordt steeds vooraf gegaan door een kort signaal
    int sommetje = 0;
    sommetje = som(volgorde);

    //wanneer de som van de array groter is dan hetgeen in de if-lus staat, 
    //dan zal in een nieuwe array worden genoteerd of er een kort of een lang signaal
    //werd gehoord
    //Na detectie van een lang signaal moeten alle waarden terug op 0 gezet worden
    //omdat er anders opnieuw een kort signaal gedetecteerd zal worden
    if (sommetje>4095*langSignaal){
      voegToeMorse(LANG);
      delay(1000); //1 seconde wachten vooraleer opnieuw gedetecteerd
                   //want array wordt op 0 gezet meteen nadat lang signaal werd gedetecteerd
                   //dus wachten, omdat anders met de 'overschot' van het lang signaal
                   //nog een kort signaal wordt gedetecteerd 
    }

    else if (sommetje>4095*kortSignaal){
      voegToeMorse(KORT);
    }
    else if (sommetje<4095*stilteSignaal){
      voegToeMorse(STIL);
    }
    vergelijk_fout();
    vergelijk();
  
    delay(2);
  }

}