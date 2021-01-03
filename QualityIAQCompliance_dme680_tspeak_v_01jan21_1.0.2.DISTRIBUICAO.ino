/* este programa monitora, registra no servidor ThingSpeak temperatura, umidade, pressão, gases voláteis e calcula o
 * indice de qualidade do ar. 
 * este programa serve como base de dados para outros dispositivos atuarem e controlarem sistemas de climatização/refrigeraço e ventialação
 *  
 *  referencias: Adafruit, Arduino, Bosh, https://github.com/G6EJD
 *  
 *  by zeh sobrinho jan 2021
 *  gastos: R$1000,00 - peças, tempo {(cinco dias batendo cabeça e correndo na santa efigência, cara pra caralho, mas faz toda diferença conversar com bons
 *  profissionais, rango, redbull)}
 *  programador normal - faz isso em pouca horas de mãos amarradas, ou seja, 1/10 do tempo e recursos
 *  meta: reduzir o código para 20 linhas usando OO programação orientada a objetos e bibliotecasa TS para arduino
 *  versão 1.0.2
 */


#include <ESP8266WiFi.h>                                                                  /* importação de bibliotecas*/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme;                                                                      /*I2C protocolo de comunicação*/ 

float hum_weighting = 0.25;                                                               /* efeito da umidade como 25% da pontuação total da qualidade do ar*/ 
float gas_weighting = 0.75;                                                               /*então o efeito do zumbido é 25% da pontuação total da qualidade do ar*/ 
int   humidity_score, gas_score;
float gas_reference = 2500;
float hum_reference = 40;
int   getgasreference_count = 0;
int   gas_lower_limit = 10000;                                                            /*Qualidade do ar ruim*/ 
int   gas_upper_limit = 300000;                                                           /*Bom limite de qualidade do ar*/ 

#define SSID_REDE "xxxxxxxxxxxxxx"                                                        /* rede de dados wifi residencial ou corporatia */
#define SENHA_REDE "xxxxxxxxxxxxx"                                                        /* coloque aqui a senha da rede que se deseja conectar */
#define INTERVALO_ENVIO_THINGSPEAK 30000                                                  /* intervalo entre envios de dados ao ThingSpeak (em ms) */
 
 

char endereco_api_thingspeak[] = "api.thingspeak.com";                                    /* constantes e variáveis globais */
String chave_escrita_thingspeak = "XXXXXXXXXXXXXXXXX";                                    /* Coloque aqui sua chave de escrita do seu canal */
unsigned long last_connection_time;
WiFiClient client;
// DHT dht(DHTPIN, DHTTYPE);
 
/* prototypes */
void envia_informacoes_thingspeak(String string_dados);
void init_wifi(void);
void conecta_wifi(void);
void verifica_conexao_wifi(void);
 
                                                                                  /*
                                                                                  * Implementações
                                                                                  */
                                                                                   
                                                                                  /* Função: envia informações ao ThingSpeak
                                                                                  * Parâmetros: String com a informação a ser enviada
                                                                                  * Retorno: nenhum
                                                                                  */
void envia_informacoes_thingspeak(String string_dados)
{
    if (client.connect(endereco_api_thingspeak, 80))
    {
        /* faz a requisição HTTP ao ThingSpeak */
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+chave_escrita_thingspeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(string_dados.length());
        client.print("\n\n");
        client.print(string_dados);
         
        last_connection_time = millis();
        Serial.println("- Informações enviadas ao ThingSpeak!");
    }
}
 
                                                                            /* Função: inicializa wi-fi
                                                                            * Parametros: nenhum
                                                                            * Retorno: nenhum
                                                                            */
void init_wifi(void)
{
    Serial.println("------WI-FI -----");
    Serial.println("Conectando-se a rede: ");
    Serial.println(SSID_REDE);
    Serial.println("\nAguarde...");
 
    conecta_wifi();
}
 
void conecta_wifi(void)
{
                                                                            /* Se ja estiver conectado, nada é feito. */
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }
     
                                                                            /* refaz a conexão */
    WiFi.begin(SSID_REDE, SENHA_REDE);
     
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
    }
 
    Serial.println("Conectado com sucesso a rede wi-fi \n");
    Serial.println(SSID_REDE);
}
 
                                                                            /* Função: verifica se a conexao wi-fi está ativa
                                                                            * (e, em caso negativo, refaz a conexao)
                                                                            * Parametros: nenhum
                                                                            * Retorno: nenhum
                                                                            */
void verifica_conexao_wifi(void)
{
    conecta_wifi();
}
 
void setup()
{
    Serial.begin(115200);
    last_connection_time = 0;
 
    
    bme.begin();                                                            /* Inicializa sensor de temperatura e umidade relativa do ar */
    Wire.begin();
  if (!bme.begin()) {
    Serial.println("Não foi possível encontrar um sensor BME680 válido, verifique a fiação!");
    while (1);
  } else Serial.println("Encontrou um sensor");
 
                                                                            /* Inicializa e conecta-se ao wi-fi */
    init_wifi();

  Serial.println("BME680 teste");
  

                                                                            /*Configurar sobre amostragem e inicialização do filtro*/ 
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);                                               /*320°C para 150 ms*/ 
                                                                            /*// Agora execute o sensor para normalizar as leituras e, em seguida, use a combinação
                                                                             * de umidade relativa e resistência do gás para estimar a qualidade do ar interno como uma porcentagem.
                                                                            */
  
  
  GetGasReference();                                                         /*O sensor leva cerca de 30 minutos para se estabilizar totalmente*/ 
   
}
 
                                                                              /*loop principal*/
void loop()
{

  Serial.println("Leituras de sensor: ");
  Serial.println("  Temperatura = " + String(bme.readTemperature(), 2)     + "°C");
  Serial.println("      Pressão = " + String(bme.readPressure() / 100.0F) + " hPa");
  Serial.println("      Umidade = " + String(bme.readHumidity(), 1)        + "%");
  Serial.println("          Gás = " + String(gas_reference)               + " ohms");
  Serial.print("Índice qualitativo de qualidade do ar ");
  Serial.println((0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00);

  humidity_score = GetHumidityScore();
  gas_score      = GetGasScore();

                                                                                                /*Combine os resultados para o valor do índice IAQ final (0-100% onde 100% é ar de boa qualidade)*/ 
  float air_quality_score = humidity_score + gas_score;
  Serial.println(" Composto de " + String(humidity_score) + "% Umidade e " + String(gas_score) + "% Gas");
  if ((getgasreference_count++) % 5 == 0) GetGasReference();
  Serial.println(CalculateIAQ(air_quality_score));
  Serial.println("--------------------------------------------------------------");
  delay(2000);
    
    char fields_a_serem_enviados[100] = {0};                                                    /* declararaçaõ de variáveis que vão subir para o ThingSpeak*/
    float temperatura_lida = 0.0;
    float umidade_lida = 0.0;
    float pressao_lida = 0.0;
    float gas_score = 0.0;
    float umidade_score = 0.0;
    float gas_referencia = 0.0;
    float erro_temperatura = 3.0;
    


    
 
                                                                                                /* Força desconexão ao ThingSpeak (se ainda estiver conectado) */
    if (client.connected())
    {
        client.stop();
        Serial.println("- Desconectado do ThingSpeak");
        Serial.println();
    }
 
                                                                                                /* Garante que a conexão wi-fi esteja ativa */
    verifica_conexao_wifi();
     
                                                                                                /* Verifica se é o momento de enviar dados para o ThingSpeak */
    if( millis() - last_connection_time > INTERVALO_ENVIO_THINGSPEAK )
    {
        temperatura_lida = (bme.readTemperature()- erro_temperatura);                          /*caraio */ 
        gas_referencia = gas_reference;
        umidade_lida = bme.readHumidity();
        pressao_lida = (bme.readPressure() / 100.0F);
        gas_score = (((0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00));
        umidade_score = humidity_score;  
        sprintf(fields_a_serem_enviados,"field1=%.2f&field2=%.2f&field3=%.2f&field4=%.2f&field5=%.2f&field6=%.2f&field7=%.2f", temperatura_lida, umidade_lida, pressao_lida, gas_score, umidade_score, air_quality_score,gas_referencia);
       
        envia_informacoes_thingspeak(fields_a_serem_enviados);                                /* sob as variaveis */
    }
 
    delay(1000);
}


void GetGasReference() {
                                                                                                /*Agora execute o sensor por um período de burn-in e, em seguida, use a combinação de umidade 
                                                                                                 * relativa e resistência ao gás para estimar a qualidade do ar interno como uma porcentagem.
                                                                                                */  
  
  Serial.println("Obtendo um novo valor de referência de gás");
  int readings = 10;
  for (int i = 1; i <= readings; i++) {                                                         /*ler gás por 10 x 0,150mS = 1,5segs*/ 
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
  
  Serial.println("Gás Referência = "+String(gas_reference,3));
}

String CalculateIAQ(int score) {
  String IAQ_text = "Qualidade do ar é ";
  score = (100 - score) * 5;
  if      (score >= 301)                  IAQ_text += "Perigosa";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Muito prejudicial à saúde";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Pouco saudável";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Insalubre para grupos de risco";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderada";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Boa";
  Serial.print("IAQ Score = " + String(score) + ", ");
  return IAQ_text;
}

int GetHumidityScore() {                                                                        /* Calcular a contribuição da umidade para o índice IAQ*/
  float current_humidity = bme.readHumidity();
  if (current_humidity >= 38 && current_humidity <= 42)                                         /* Umidade +/- 5% em torno do ideal */
    humidity_score = 0.25 * 100;
  else
  {                                                                                             /*A umidade está abaixo do ideal*/ 
    if (current_humidity < 38)
      humidity_score = 0.25 / hum_reference * current_humidity * 100;
    else
    {
      humidity_score = ((-0.25 / (100 - hum_reference) * current_humidity) + 0.416666) * 100;
    }
  }
  return humidity_score;
}

int GetGasScore() {
                                                                                                /*Calcule a contribuição do gás para o índice IAQ*/ 
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00;
  if (gas_score > 75) gas_score = 75;                                                           /*Às vezes, as leituras de gás podem sair da escala máxima esperada*/
  if (gas_score <  0) gas_score = 0;  
  return gas_score;
}
