## Sistema de Frequência - Projeto de Laboratório Inteligente

Este projeto tem como objetivo criar um sistema de registro de frequência para um laboratório utilizando tecnologia RFID. Ele permite a leitura de tags RFID, enviando os dados para uma API via MQTT. O sistema é baseado em um ESP8266 (NodeMCU 12S) e integra um módulo RFID MFRC522, LEDs para feedback visual, buzzer para feedback sonoro e um botão para controle de modo.

---

### Prototipação da Placa

O esquemático e o design da PCB estão disponíveis no EasyEDA:

[Abrir projeto no EasyEDA](https://easyeda.com/editor#id=b40ef3a094834cf3b335c5ce041ecf82|c5e54f14bb704e79859851ce249f2990|cfe3a18bf16944a8aa74a8a247297beb)

---

### Hardware

#### Microcontrolador

| Componente | Modelo |
|---|---|
| Microcontrolador | NodeMCU 12S (ESP8266) |
| Tensão de operação | 3.3V (regulador interno), alimentação via USB 5V |

#### Módulo RFID

| Componente | Modelo | Conexão |
|---|---|---|
| Leitor RFID | MFRC522 | Barramento SPI |

| Pino RFID | Pino NodeMCU | GPIO |
|---|---|---|
| SDA (SS) | D2 | GPIO 4 |
| RST | D1 | GPIO 5 |
| SCK | D5 (CLK) | GPIO 14 |
| MOSI | D7 | GPIO 13 |
| MISO | D6 | GPIO 12 |
| VCC | 3.3V | — |
| GND | GND | — |

#### LEDs

| LED | Cor | Função | GPIO | Pino NodeMCU | Resistor |
|---|---|---|---|---|---|
| LED1 | Verde | Indica sucesso (frequência registrada) | GPIO 16 | D0 | 1kΩ |
| LED2 | Vermelho | Indica erro ou aguardando leitura | GPIO 0 | D3 | 1kΩ |

#### Buzzer

| Componente | Função | GPIO | Pino NodeMCU |
|---|---|---|---|
| Buzzer (BUZZER1) | Feedback sonoro (sucesso / erro / timeout) | GPIO 2 | D4 |

#### Botão

| Componente | Função | GPIO | Pino NodeMCU | Resistor |
|---|---|---|---|---|
| Botão (KEY2) | Alterna entre modo **cadastro** e **frequência** | GPIO 15 | D8 | 1kΩ (pull-down) |

---

### Modos de Operação

O sistema possui dois modos, controlados pelo botão:

| Estado do botão | Modo | Descrição |
|---|---|---|
| **HIGH** (pressionado) | Cadastro | Vincula a tag RFID ao último usuário criado na API |
| **LOW** (solto) | Frequência | Registra a presença do aluno associado à tag |

---

### Comunicação MQTT

O ESP8266 se comunica com a API REST via um broker MQTT. Abaixo estão os tópicos utilizados:

#### Tópicos de Envio (ESP → Servidor)

| Tópico | Modo | Payload | Descrição |
|---|---|---|---|
| `gph/cadastrar/rfid` | Cadastro | UID da tag (ex: `"A1B2C3D4"`) | Envia o UID para **cadastrar a tag RFID**. A tag é vinculada automaticamente ao último usuário criado no sistema. |
| `gph/cadastrar/frequencia` | Frequência | UID da tag (ex: `"A1B2C3D4"`) | Envia o UID para **registrar a presença** do aluno associado àquela tag. |

#### Tópicos de Recebimento (Servidor → ESP)

| Tópico | Resposta | Descrição |
|---|---|---|
| `gph/cadastrar/rfid/output` | `"ok"` ou mensagem de erro | Confirmação do cadastro da tag. |
| `gph/cadastrar/frequencia/output` | `"ok"` ou mensagem de erro | Confirmação do registro de frequência. |

> **Nota:** O payload enviado é apenas a string do UID em hexadecimal, sem formatação adicional.

---

### Feedbacks do Sistema

| Evento | LED | Buzzer |
|---|---|---|
| Aguardando leitura | Vermelho aceso | — |
| Frequência registrada com sucesso | Verde pisca 5x | Melodia ascendente (Dó-Mi-Sol-Dó) |
| Erro na confirmação | Vermelho pisca 5x | 3 bips graves |
| Timeout (sem resposta em 5s) | — | 3 bips graves |

---

### Configuração

Antes de gravar o firmware, altere as seguintes constantes no arquivo `frequency.c`:

```c
const char* ssid = "wifi";            // Nome da rede Wi-Fi
const char* password = "senha";       // Senha da rede Wi-Fi
const char* mqtt_server = "IP";       // Endereço do broker MQTT
const char* mqtt_auth = "usuario_mqtt"; // Usuário do broker MQTT
const char* mqtt_pass = "senha_mqtt";   // Senha do broker MQTT
```

A porta MQTT utilizada é **1884**.
