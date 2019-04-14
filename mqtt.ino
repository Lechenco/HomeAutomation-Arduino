//Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
//Autor: Pedro Bertoleti
 
#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
 
//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_PUBLISH   "channels/633522/publish/W29GUSS799YXFUNO" //tópico MQTT de envio de informações para Broker
                                                   
//#define ID_MQTT  "HomeAut"     //id mqtt (para identificação de sessão)
                                                   
 
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1
 
 
// WIFI
const char* SSID = "My ASUS"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "12345678"; // Senha da rede WI-FI que deseja se conectar
  
// MQTT
const char* BROKER_MQTT = "mqtt.thingspeak.com"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
const char* MQTT_KEY = "J8SYWSTCG8X48QHD"; 
const char* SUBSCRIBE_TOPIC = "channels/633521/subscribe/fields/+/S16YVPSSWEZYW9J4";


//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
int fields[8];
int mem_field[8];
int count = 0;
boolean onWait = false;
  
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
void updateOutput();
void changeLamp(int lamp, int field);
void jingleBell();
 
/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    reconnectMQTT();
}
  
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    reconectWiFi();
}
  
//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado    
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
  
//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
   
    Serial.println(topic);
    Serial.println(msg);
    
    fields[topic[38] - 0x30 -1] = msg.toInt();

    count++;
    if(count == 5){
        count = 0;
        updateOutput();
        if(!onWait) {
          delay(1000);
          EnviaEstadoOutputMQTT();
          
        } else
          onWait = false;
     }
     
}
  
//Função: atualiza as pinagens de saída
void updateOutput() 
{
    int i;
    for(i = 0; i <= 8; i++){
        if(fields[i] != mem_field[i]) {
            switch(i){
                //Lampadas
                case 1: case 2: case 3: case 4:
                    changeLamp(i, fields[i]);
                    break;
                
            }

            mem_field[i] = fields[i];
        }
    }

}

//Funcção: muda o estado da lampada indicada pelo parametro
//Parâmetros: lamp - numero da lâmpada 
void changeLamp(int lamp, int field) 
{
  Serial.println("Lampada: " + String(lamp));
    switch(lamp) {
        case 1: 
            digitalWrite(D0, field == 1? HIGH: LOW);
            //digitalWrite(D0, HIGH);

            //delay(2000);

            //digitalWrite(D0, LOW);
            break;

        case 2:
            digitalWrite(D1, field == 1? HIGH : LOW);
            //digitalWrite(D1, HIGH);

            //delay(2000);

            //digitalWrite(D1, LOW);
            break;

        case 3:
            digitalWrite(D2, field == 1? HIGH: LOW);
            //digitalWrite(D0, HIGH);
            //digitalWrite(D1, HIGH);

            //delay(2000);

            //digitalWrite(D0, LOW);
            //digitalWrite(D1, LOW);
            break;

        case 4:
            digitalWrite(D3, field == 1? HIGH: LOW);
            //digitalWrite(D2, HIGH);

            //delay(2000);

            //digitalWrite(D2, LOW);
            break;
    }
}

//Função: interrupção para quando a campainha for tocada
void jingleBell() 
{
  if(digitalRead(D4) == HIGH) {
    delay(500);
    while(digitalRead(D4) == HIGH) {
      yield();   //atualiza o watchDog  
    };
      Serial.println("TRIMMM!!!!");
      delay(5000);
      fields[5] = 1;
      EnviaEstadoOutputMQTT();
      onWait = true;
      delay(500);
      fields[5] = 0;
  }
    
    
}


//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect("123", "nodeMCU", MQTT_KEY)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(SUBSCRIBE_TOPIC, 0); 
           // EnviaEstadoOutputMQTT();
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
     
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
 
//Função: envia ao Broker o estado atual do output 
//Parâmetros: nenhum
//Retorno: nenhum
void EnviaEstadoOutputMQTT(void)
{
    String msg;
    msg += "field1=" + String(fields[0]);
    msg += "&field2=" + String(fields[1]);
    msg += "&field3=" + String(fields[2]);
    msg += "&field4=" + String(fields[3]);
    msg += "&field5=" + String(fields[4]);
    msg += "&field6=" + String(fields[5]);
    msg += "&status=MQTTPUBLISH";
    char payload[100];
    msg.toCharArray(payload, 100);

    MQTT.publish(TOPICO_PUBLISH, payload);

 
    Serial.println(msg);
    delay(1000);
}
 
//Função: inicializa o output em nível lógico baixo
//Parâmetros: nenhum
//Retorno: nenhum
void InitOutput(void)
{
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(D0, OUTPUT);    //D0 D1 D2 pinos para o controle do multiplexador
    pinMode(D1, OUTPUT);
    pinMode(D2, OUTPUT);
    pinMode(D3, OUTPUT);
    pinMode(D4, INPUT);     //D4 pino para a  campainha


    digitalWrite(D0, LOW);          
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);

   // attachInterrupt(digitalPinToInterrupt(D4), jingleBell, RISING);
}
 
 
//programa principal
void loop() 
{   
    //garante funcionamento das conexões WiFi e ao broker MQTT
    VerificaConexoesWiFIEMQTT();
 
    //envia o status de todos os outputs para o Broker no protocolo esperado
   // EnviaEstadoOutputMQTT();
    //jingleBell();
    
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}
