//  =================================================================================================================================================
//  --- Bibliotecas ---
#include <EEPROM.h>
#include <SoftwareSerial.h>

//  =================================================================================================================================================
//  --- Funções ---
void incpulso();
void recuperar(float* Litros);
void zerar(float* Litros, float* MiliLitros);
void salvar(float Litros);

//  =================================================================================================================================================
//  --- Definição dos pinos utilizados ---
#define SENSOR 2 //Utiliza o pino 2 do arduino para fazer a leitura dos dados do sensor
#define RX 10 //Utiliza o pino 10 do arduino como RX
#define TX 11 //Utiliza o pino 11 do arduino como TX

//  =================================================================================================================================================
//  --- Variáveis ---
//  Variáveis usadas para contabilizar o volume de água
float vazao; //Variável para armazenar o valor em L/min
int contaPulso; //Variável para a quantidade de pulsos
float Litros = 0; //Variável para Quantidade de agua
float MiliLitros = 0; //Variavel para Conversão

//   Variáveis usadas para salvar dados no arduino
int addr = 0; //Variável para endereço da memória
bool flagSalvar = 0; //Variável que indica se precisa salvar ou não

//   Variáveis usadas para utilização do bluetooth
SoftwareSerial bluetooth(RX, TX); //Bluetooth
String comando; //Variável que armazena os comandos recebidos


//  =================================================================================================================================================
//  --- Setup ---
void setup()  {
 Serial.begin(9600);
 bluetooth.begin(9600);

 pinMode(SENSOR, INPUT);
 attachInterrupt(0, incpulso, RISING); //Configura o pino 2(Interrupção 0) interrupção

   recuperar(&Litros);
}


//  =================================================================================================================================================
//  --- Loop principal --- 
void loop ()  {  
 contaPulso = 0;//Zera a variável
 sei(); //Habilita interrupção
 delay (1000); //Aguarda 1 segundo
 cli(); //Desabilita interrupção
 
 vazao = contaPulso / 5.5; //Converte para L/min
 MiliLitros = (vazao / 60) - (MiliLitros*0.2); //Diminuindo 20% do valor para minimizar o erro. Baseado em testes
 Litros = Litros + MiliLitros;

  if(vazao >= 0.1)  //Se teve alguma vazão quer dizer que os dados vão precisar ser salvos
     flagSalvar = true;

  else  //Se não tem vazão
  {
    if(flagSalvar) //Se a flag for true, significa que houve vazão e os dados estão precisando ser salvos
      salvar(Litros);
      
    flagSalvar = false; //Se não tem vazão ou se os dados já foram salvos, não é necessário salvar
  }

  comando = ""; //Armazena os comandos vindos do bluetooth

  if(bluetooth.available()) //Faz a leitura dos dados enviados pelo bluetooth
  {
    while(bluetooth.available())
    {
      char caracter = bluetooth.read();

      comando += caracter;
      delay(10);
    }

    if(comando.indexOf("solicitarDados") >= 0) //Se os dados forem solicitados
    {
      int quantia = Litros * 1000;
      char mlEnviar[10];

      bluetooth.println("{"); //Para conferir se houve corrompimento da informação enviada
      
      sprintf(mlEnviar, "%i", quantia); 
      bluetooth.print(mlEnviar); //Envia a quantidade atual em ml

      bluetooth.println("}"); //Para conferir se houve corrompimento da informação enviada
    }

    if(comando.indexOf("zerar") >= 0) //Se for solicitado que os dados sejam apagados
    {
      salvar(0);  //Apaga os dados
      zerar(&Litros, &MiliLitros);
    }
  }
}


//  =================================================================================================================================================
//  --- Incrementar Pulso ---
void incpulso() 
{
 contaPulso++; //Incrementa a variável de pulsos
}


//  =================================================================================================================================================
//  --- Salvar na EEPROM ---
void salvar(float Litros)
{
   int mlSalvar;
   mlSalvar = Litros * 1000;

   byte hiByte = highByte(mlSalvar);  //Divide os dados em 2 bytes, para poder salvar números maiores que 255
   byte loByte = lowByte(mlSalvar);

   EEPROM.write(addr, hiByte);  //Salva os dados em 2 bytes consecutivos
   EEPROM.write(addr+1, loByte);
}


//  =================================================================================================================================================
//  --- Zerar Variáveis ---
void zerar(float* Litros, float* MiliLitros)
{
    *MiliLitros = 0;
    *Litros     = 0;
}


//  =================================================================================================================================================
//  --- Recuperar dados ---
void recuperar(float* Litros)
{
   int valor;

   byte hiByte = EEPROM.read(addr); //Lê os 2 bytes e recupera a informação previamente salva
   byte loByte = EEPROM.read(addr+1);

   valor = word(hiByte, loByte);

   *Litros = (float)((valor) / 1000.0);
}
