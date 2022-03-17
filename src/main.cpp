#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Arduino.h>

// Update these with values suitable for your network.

const char *ssid = "UPBWiFi";
const char *password = "";
const char *mqtt_server = "10.8.153.208";

#define BUTTON_LEFT 0
#define DHTPIN 27
#define DHTTYPE DHT11
#define LEDPin1 26
#define LEDPin2 25

int led1 = 0;
int led2 = 0;

DHT dht(DHTPIN, DHTTYPE);
TFT_eSPI tft = TFT_eSPI();

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool toggle = true;



void isr()
{
  if(toggle)
  {
    Serial.println("On");
  }
  else
  {
    Serial.println("Off");
  }

  toggle = !toggle;
}

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Conectando a el servidor:");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msgtemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    msgtemp += (char)payload[i];
  }
  Serial.println();

  //Switch 1
  if(String(topic)=="esp32/led1")
  {
    if(msgtemp=="1")
    {
      Serial.println("led1 on");
      tft.fillCircle(10, 10, 10, TFT_RED); //(X,Y,radio,color)
      digitalWrite(LEDPin1, HIGH);
    }
    else
    {
      Serial.println("led1 off");
      tft.fillCircle(10, 10, 10, TFT_DARKGREY); //(X,Y,radio,color)
      digitalWrite(LEDPin1, LOW);
    }
  }

  //Switch 2
    if(String(topic)=="esp32/led2")
  {
    if(msgtemp=="1")
    {
      Serial.println("led2 on");
      tft.fillCircle(120, 10, 10, TFT_GREEN); //(X,Y,radio,color)
      digitalWrite(LEDPin2, HIGH);
    }
    else
    {
      Serial.println("led2 off");
      tft.fillCircle(120, 10, 10, TFT_DARKGREY); //(X,Y,radio,color)
      digitalWrite(LEDPin2, LOW);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Intentando connexion MQTT...");
    // Create a random client ID
    String clientId = "ESP32Client1";
    // Attempt to connect
    if (client.connect("ESP32Client1"))
    {
      Serial.println("conectado");
      // ... and subscribe
      client.subscribe("esp32/led1");
      client.subscribe("esp32/led2");
    }
    else
    {
      Serial.print("Conexion fallida, rc=");
      Serial.print(client.state());
      Serial.println("Intentando en 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(LEDPin1, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  pinMode(LEDPin2, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  tft.init();
  tft.fillScreen(0x0000);
  dht.begin();

  attachInterrupt(BUTTON_LEFT, isr, RISING);
  
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  delay(500);
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  tft.drawString("Temp. (°C):", 0, 30, 4);
  tft.drawString(String(t), 0, 60, 7);

  tft.drawString("HMDD (%):", 0, 130, 4);
  tft.drawString(String(h), 0, 160, 7);

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;

    char hstring[3];
    dtostrf(h, 2, 0, hstring);
    Serial.print("(%) HMDD: ");
    Serial.println(hstring);
    client.publish("esp32/hmdd", hstring);

    char tstring[3];
    dtostrf(t, 2, 0, tstring);
    Serial.print("(°C) Temp: ");
    Serial.println(tstring);
    Serial.println("");
    client.publish("esp32/temp", tstring);

    char swstring[20];
    //dtostrf(t, 2, 0, swstring);
    sprintf(swstring, "%d", toggle);
    Serial.print("SW estado: ");
    Serial.println(swstring);
    Serial.println("");
    client.publish("esp32/sw", swstring);
  }
}
// mosquitto.exe -v -c test.conf
// node-red