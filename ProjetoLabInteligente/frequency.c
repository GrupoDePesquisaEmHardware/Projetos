// Bibliotecas
#include <SPI.h>       
#include <MFRC522.h>     
#include <string.h>       
#include <ESP8266WiFi.h>   
#include <PubSubClient.h>  

// Mapeamento de pinos do NodeMCU
#define SS_PIN 4    // D2 - Pino SDA do módulo RFID
#define RST_PIN 5   // D1 - Pino de reset do módulo RFID
#define LED1 16     // D0 - LED Verde (indica sucesso/frequência registrada)
#define LED2 0      // D3 - LED Vermelho (indica erro ou aguardando leitura)
#define BUZZER 2    // D4 - Buzzer para feedback sonoro
#define BUTTON 15   // Botão para alternar entre modo cadastro e frequência

// Frequências das notas musicais para o buzzer (em Hz)
#define NOTE_C5 523    // Dó
#define NOTE_E5 659    // Mi
#define NOTE_G5 784    // Sol
#define NOTE_C6 1047   // Dó
#define NOTE_ERROR 150 // Tom grave (usado para indicar erro)

// Incializa o objeto do leitor RFID e o cliente MQTT
MFRC522 mfrc522(SS_PIN, RST_PIN);  
WiFiClient espClient;             
PubSubClient client(espClient);  

// Variáveis de controle
int send = 0;                     // Indica envio do UID
int confirm = 0;                  // Indica recebimento da confirmação do servidor
unsigned long tempoAnterior = 0;  // Marca o tempo da última ação (para controle de timeout)
const long intervalo = 5000;      // Tempo máximo de espera pela confirmação

// Credenciais
const char* ssid = "wifi";
const char* password = "senha";
const char* mqtt_server = "IP";
const char* mqtt_auth = "usuario_mqtt";
const char* mqtt_pass = "senha_mqtt";

// Tópicos MQTT
const char* topicoCadastro = "gph/cadastrar/rfid";                   // Envia UID para cadastrar nova tag
const char* topicoFrequencia = "gph/cadastrar/frequencia";           // Envia UID para registrar frequência
const char* topicoCadConfirm = "gph/cadastrar/rfid/output";          // Resposta do cadastro
const char* topicoFreqConfirm= "gph/cadastrar/frequencia/output";    // Resposta da frequência

// Variáveis auxiliares
int estadoBTN;  // Estado atual do botão (1 = cadastro, 0 = frequência)
int topico;     // Modo selecionado: 1 = cadastro, 2 = frequência

char ID[20];                                              // Vetor para armazenar o UID do cartão lido
int melodia[] = {  NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6 };  // Notas da melodia de sucesso
int duracoes[] = { 200,     200,     200,     400   };    // Duração de cada nota (ms)

// Função de callback para processar mensagens recebidas do broker MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; 
  String mensagem = String((char*)payload);

  // Verifica se a mensagem veio do tópico de confirmação de frequência
  if (String(topic) == topicoFreqConfirm) {
    if (mensagem == "ok") {
      // Se a mensagem foi confirmada, emite feedback de sucesso
      digitalWrite(LED1,HIGH);

      for(int i = 0;i<=5;i++){
        digitalWrite(LED1,HIGH);
        delay(100);
        digitalWrite(LED1,LOW);
        delay(100);
      }
    
      for (int i = 0; i < 4; i++) {
        tone(BUZZER, melodia[i]);
        delay(duracoes[i]);
        noTone(BUZZER);
        delay(50);
      }

      digitalWrite(LED1, LOW);

      confirm = 1; // Sinaliza que a confirmação foi recebida com sucesso

      Serial.println("Confirmação OK recebida!");
      delay(500);
    } else {
      // Se a mensagem não foi recebida, indica erro e emite feedback de falha
      Serial.println("Erro recebido na confirmação.");

      for(int i = 0;i<=5;i++){
        digitalWrite(LED2,HIGH);
        delay(100);
        digitalWrite(LED2,LOW);
        delay(100);
      }

      // Toca 3 bips graves indicando erro
      for (int i = 0; i < 3; i++) {
        tone(BUZZER, NOTE_ERROR);
        delay(200);
        noTone(BUZZER);
        delay(100);
      }

      delay(500);
    }
    send = 0; // Libera o sistema para uma nova leitura de cartão
  }
}


// Conecta o ESP à rede Wi-Fi usando as credenciais fornecidas
void setup_wifi() {
  delay(10);
  Serial.println();
  WiFi.begin(ssid, password); // Inicia a tentativa de conexão Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); // Imprime pontos enquanto aguarda conexão
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP()); // Exibe o IP atribuído ao ESP na rede
}

// Reconecta o ESP ao broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP8266Client123", mqtt_auth, mqtt_pass)) {
      Serial.println("conectado");
      // Inscreve nos tópicos de resposta para ouvir confirmações do servidor
      client.subscribe(topicoCadConfirm);
      client.subscribe(topicoFreqConfirm);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

// Iniciliza o sistema
void setup() {
  Serial.begin(115200); // Inicia comunicação serial para debug no Monitor Serial

  // Configura os pinos de entrada e saída
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP); 

  // Realiza conexões e configurações iniciais
  setup_wifi();                      
  client.setServer(mqtt_server, 1884);  
  client.setCallback(callback);         
  SPI.begin();                          
  mfrc522.PCD_Init();               
  Serial.println("Aproxime o cartão RFID...");
}

// Loop principal do programa
void loop() {
  unsigned long tempoAtual = millis();
  digitalWrite(LED2,HIGH); // LED vermelho aceso indica "pronto para leitura"

  // Garante que a conexão MQTT esteja ativa
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Processa mensagens MQTT

  // Verifica se há um cartão presente no leitor
  if (!!mfrc522.PICC_IsNewCardPresent()) {
    return; // Se não houver cartão, retorna para o início do loop
  } 

  // Lê o botão para definir o modo de operação
  estadoBTN = digitalRead(BUTTON);
  if (estadoBTN){
    topico = 1; // Modo cadastro
  } else {
    topico = 2; // Modo frequência
  }

  // Leitura do cartão RFID e envio do UID
  if (send == 0){ // Só lê se não estiver aguardando confirmação
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      digitalWrite(LED2,LOW); // Apaga LED vermelho ao detectar cartão

      Serial.print("UID da tag: ");
      ID[0] = '\0';

      // Converte o UID do cartão para uma string hexadecimal
      for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0"); 
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        char byteString[3];
        sprintf(byteString, "%02X", mfrc522.uid.uidByte[i]);
        strcat(ID, byteString);
      }

      // Publica o UID no tópico MQTTo
      if (topico == 1){
        client.publish(topicoCadastro,ID);    // Envia para o tópico de cadastro
      } else if (topico == 2){
        client.publish(topicoFrequencia,ID);  // Envia para o tópico de frequência
      }

      delay(1000);

      send = 1; // Marca que um UID foi enviado e aguarda confirmação
    } else {
      digitalWrite(LED1,LOW);
      digitalWrite(LED2,HIGH);
    }
    tempoAnterior = tempoAtual; // Registra o momento do envio para controle de timeout
  }

  // Reseta a flag de confirmação quando não há envio pendente
  if (send == 0 and confirm == 1){
    confirm = 0;
  }

  // Se houver timeout
  if(send && confirm == 0){
    if (tempoAtual - tempoAnterior >= intervalo) {
      tempoAnterior = tempoAtual;
      // Emite 3 bips de erro para indicar que o servidor não respondeu a tempo
      for (int i = 0; i < 3; i++) {
        tone(BUZZER, NOTE_ERROR);
        delay(200);
        noTone(BUZZER);
        delay(100);
        send = 0; // Libera para nova leitura
      }
    }
  }

  delay(1000);
}
