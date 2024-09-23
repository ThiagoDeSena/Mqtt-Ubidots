#include "DHT.h"
#include "MQTT_Client.h" //Arquivo com as funções de mqtt
#define DHTPIN 13 // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11

#include <WiFi.h>
#include <PubSubClient.h> //Biblioteca para as publicações via mqtt

#define WIFISSID "ifce-espacoMaker" //Coloque seu SSID de WiFi aqui
#define PASSWORD "CR1AT1V1UM" //Coloque seu password de WiFi aqui
#define TOKEN "BBUS-GEAtmIzZ1sMrBcAwZh9OyiLpXqTRG1" //Coloque seu TOKEN do Ubidots aqui
#define VARIABLE_LABEL_TEMPERATURE "temperature" //Label referente a variável de temperatura criada no ubidots
#define VARIABLE_LABEL_HUMIDITY "humidity" //Label referente a variável de umidade criada no ubidots
#define DEVICE_ID "66f1af28986f11cd46d83705" //ID do dispositivo (Device id, também chamado de client name)
#define SERVER "things.ubidots.com" //Servidor do Ubidots (broker)

//Porta padrão
#define PORT 1883

//Tópico aonde serão feitos os publish, "esp32-dht" é o DEVICE_LABEL
#define TOPIC "/v1.6/devices/esp32-kincony"

//Objeto WiFiClient usado para a conexão wifi
WiFiClient ubidots;
//Objeto PubSubClient usado para publish–subscribe
PubSubClient client(ubidots);

float temperature; //Temperatura que será obtida pelo sensor DHT
float humidity; //Umidade que será obtida pelo sensor DHT

// const int pinDHT = 13; //Pino que é ligado no sensor DHT
// SimpleDHT22 dht22(pinDHT); //Objeto que possui os métodos de leitura dos v

 
DHT dht(DHTPIN, DHTTYPE);

bool mqttInit();
void reconnect();
 
void setup() 
{
  Serial.begin(115200);
  Serial.println("DHTxx test!");
  dht.begin();

  Serial.println("Setting up mqtt...");
  //Inicializa mqtt (conecta o esp com o wifi, configura e conecta com o servidor da ubidots)
  if(!mqttInit())
  {        
    delay(3000);
    
    Serial.println("Failed!");
    ESP.restart();
  }
  
  Serial.println("OK");
}
 
void loop() 
{
  //Se o esp foi desconectado do ubidots, tentamos reconectar
  if(!client.connected())
    reconnect();


  // A leitura da temperatura e umidade pode levar 250ms!
  // O atraso do sensor pode chegar a 2 segundos.
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // testa se retorno é valido, caso contrário algo está errado.
  if (isnan(t) || isnan(h)) 
  {
    Serial.println("Failed to read from DHT");
  } 
  else
  {
    Serial.print("Umidade: ");
    Serial.print(h);
    Serial.print(" %t");
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println(" *C");
  }

  //Esperamos 2.5s antes de exibir o status do envio para dar efeito de pisca no display
  delay(2500);  
  if(sendValues(t, h))
  {      
    Serial.println("Successfully sent data");
  }
  else
  {      
    Serial.println("Failed to send sensor data");
  }    
    
  //Esperamos 2.5s para dar tempo de ler as mensagens acima
  delay(2500); 
}

bool mqttInit()
{
  //Inicia WiFi com o SSID e a senha
  WiFi.begin(WIFISSID, PASSWORD);
 
  //Loop até que o WiFi esteja conectado
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
 
  //Exibe no monitor serial
  Serial.println("Connected to network");

  //Seta servidor com o broker e a porta
  client.setServer(SERVER, PORT);
  
  //Conecta no ubidots com o Device id e o token, o password é informado como vazio
  while(!client.connect(DEVICE_ID, TOKEN, ""))
  {
      Serial.println("MQTT - Connect error");
      return false;
  }

  Serial.println("MQTT - Connect ok");
  return true;
}


void reconnect() 
{  
  //Loop até que o MQTT esteja conectado
  while (!client.connected()) 
  {
    //sinaliza desconexão do mqtt no display
    Serial.println("Attempting MQTT connection...");
    
    //Tenta conectar
    if (client.connect(DEVICE_ID, TOKEN,"")) 
      Serial.println("connected");
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      //Aguarda 2 segundos antes de retomar
      delay(2000);
    }
  }
  //Sinaliza reconexão do mqtt no display
  Serial.println("Reconnected");
  
}


//Envia valores por mqtt
//Exemplo: {"temperature":{"value":24.50, "context":{"temperature":24.50, "humidity":57.20}}}
bool sendValues(float temperature, float humidity)
{
  char json[250];
 
  //Atribui para a cadeia de caracteres "json" os valores referentes a temperatura e os envia para a variável do ubidots correspondente
  sprintf(json,  "{\"%s\":{\"value\":%02.02f, \"context\":{\"temperature\":%02.02f, \"humidity\":%02.02f}}}", VARIABLE_LABEL_TEMPERATURE, temperature, temperature, humidity);  

  if(!client.publish(TOPIC, json))
    return false;

  //Atribui para a cadeia de caracteres "json" os valores referentes a umidade e os envia para a variável do ubidots correspondente
  sprintf(json,  "{\"%s\":{\"value\":%02.02f, \"context\":{\"temperature\":%02.02f, \"humidity\":%02.02f}}}", VARIABLE_LABEL_HUMIDITY, humidity, temperature, humidity);  
      
  if(!client.publish(TOPIC, json))
    return false;

  //Se tudo der certo retorna true
  return true;
}
