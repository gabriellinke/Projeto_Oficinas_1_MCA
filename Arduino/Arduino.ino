
#include <EEPROM.h>
#include <SoftwareSerial.h>

void recuperar();
void zerar();
void salvar();

#define RX 10 //Utiliza o pino 10 do arduino como RX
#define TX 11 //Utiliza o pino 11 do arduino como TX

byte statusLed       = 13;
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;


// The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
float calibrationFactor = 4.26;
volatile byte pulseCount;  
int pulses;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

//   Variáveis usadas para salvar dados no arduino
int addr = 0; //Variável para endereço da memória
bool flagSalvar = false; //Variável que indica se precisa salvar ou não

//   Variáveis usadas para utilização do bluetooth
SoftwareSerial bluetooth(RX, TX); //Bluetooth
String comando; //Variável que armazena os comandos recebidos

void setup()
{
  
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
  bluetooth.begin(9600);
   
  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  pulses            = 0;

  recuperar();
}

/**
 * Main program loop
 */
void loop()
{
   
   if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = (float)(((1000.0 / (millis() - oldTime)) * pulseCount)) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    unsigned int frac;
    pulses += pulseCount;
    
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(flowRate);  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\n");    

    Serial.print("Pulse in a second: ");
    Serial.println(pulseCount);  // Print number of pulses
    Serial.print("Total pulses: ");
    Serial.println(pulses);  // Print number of total pulses

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");        
    Serial.print(totalMilliLitres);
    Serial.print("mL"); 
    Serial.print("   ");       // Print tab space
    Serial.print(totalMilliLitres/1000);
    Serial.println("L\n");
    
    if(flowRate >= 0.1)  //Se teve alguma vazão quer dizer que os dados vão precisar ser salvos
       flagSalvar = true;
  
    else  //Se não tem vazão
      if(flagSalvar) //Se a flag for true, significa que houve vazão e os dados estão precisando ser salvos
        {
          salvar();
          flagSalvar = false; //Se os dados já foram salvos, não é mais necessário salvar
        }
    
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
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
      int quantia = totalMilliLitres;
      char mlEnviar[10];

      bluetooth.println("{"); //Para conferir se houve corrompimento da informação enviada
      
      sprintf(mlEnviar, "%i", quantia); 
      bluetooth.print(mlEnviar); //Envia a quantidade atual em ml

      bluetooth.println("}"); //Para conferir se houve corrompimento da informação enviada
      Serial.println("\nsolicitarDados: ");
      Serial.print(totalMilliLitres);
      Serial.println("\n");
    }
    
    if(comando.indexOf("zerar") >= 0) //Se for solicitado que os dados sejam apagados
    {
      zerar();
      salvar();
      Serial.println("\nzerar: ");
      Serial.print(totalMilliLitres);
      Serial.println("\n");
    }
  }
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

//  =================================================================================================================================================
//  --- Salvar na EEPROM ---
void salvar()
{
   byte hiByte = highByte(totalMilliLitres);  //Divide os dados em 2 bytes, para poder salvar números maiores que 255
   byte loByte = lowByte(totalMilliLitres);

   EEPROM.write(addr, hiByte);  //Salva os dados em 2 bytes consecutivos
   EEPROM.write(addr+1, loByte);
}


//  =================================================================================================================================================
//  --- Zerar Variáveis ---
void zerar()
{
    totalMilliLitres = 0;
    flowMilliLitres  = 0;
}


//  =================================================================================================================================================
//  --- Recuperar dados ---
void recuperar()
{
   byte hiByte = EEPROM.read(addr); //Lê os 2 bytes e recupera a informação previamente salva
   byte loByte = EEPROM.read(addr+1);

   totalMilliLitres = word(hiByte, loByte);
}
