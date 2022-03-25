#include <Wire.h>
#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
#include <SPI.h>
#include <LoRa.h>


//Objects
SerLCD lcd; 

//Const
static const uint8_t  AXIS0 =  A3;
static const uint8_t  AXIS1 =  A2;
static const uint8_t  AXIS2 =  A4;
static const uint8_t  BTN_JS =  A10;
static const uint8_t  BTN_F1 =  A12;
static const uint8_t  BTN_F2 =  A11;
static const uint8_t  BTN_F3 =  21;
static const uint8_t  BTN_F4 =  TX;
static const uint8_t  BTN_F5 =  A9;
static const uint8_t  BTN_F6 =  A7;
static const uint8_t  BTN_F7 =  A6;


static const uint8_t  LORA_TXEN =  A0;
static const uint8_t  LORA_RXEN =  A1;
static const uint8_t  LORA_CS =  RX;
static const uint8_t  LORA_RST =  -1;
static const uint8_t  LORA_ISR =  A5;
static const uint8_t  BAT_VT =  A13;

static const long lcdRefreshInterval = 0;

static const long frequency = 915E6;



//Vars
bool btn_f1  = 0;
bool btn_f2  = 0;
bool btn_f3  = 0;
bool btn_f4  = 0;
bool btn_c1  = 0;
bool btn_c2  = 0;
bool btn_c3  = 0;
bool btn_j1  = 0;

bool btn_l_f1  = 0;
bool btn_l_f2  = 0;
bool btn_l_f3  = 0;
bool btn_l_f4  = 0;
bool btn_l_c1  = 0;
bool btn_l_c2  = 0;
bool btn_l_c3  = 0;
bool btn_l_j1  = 0;

bool boostMode  = false;

float axis_x  = 0;
float axis_y  = 0;
float axis_z  = 0;

volatile bool LCDRefreshFlag = false; 

uint8_t failedCounter  = 0;
uint32_t lastTXTime = 0;
uint32_t txLatency = 0;
uint32_t lcdRefreshLineTimeStamp = 0;
uint8_t lcdRefreshLineTurn = 0;

byte lcdBrightness = 50;

String message0Last = "";
String message1Last = "";
String message2Last = "";
String message3Last = "";


float batteryVoltage = 0.0;
float batteryFraction = 0.0;

typedef struct bcast_message {  
  byte sourceID;
  uint8_t msgId;
  float x;
  float y;
  float z;  
  bool b1;
  bool b2;
  bool b3;
  bool bj;      
} bcast_message;

bcast_message bcastData;

void readBattery();
void onReceive(int packetSize);
void sendDataToCore(bcast_message message);


String TXStatus = "NA";

//Prototypes
String barDisplay(float volts);
void broadcastData();
void readBattery();


void LCDTask();
void updateButtonStates();



void setup() {
  pinMode(BTN_JS, INPUT_PULLDOWN);
  pinMode(BTN_F1, INPUT_PULLDOWN);
  pinMode(BTN_F2, INPUT_PULLDOWN);
  pinMode(BTN_F3, INPUT_PULLDOWN);
  pinMode(BTN_F4, INPUT_PULLDOWN);
  pinMode(BTN_F5, INPUT_PULLDOWN);
  pinMode(BTN_F6, INPUT_PULLDOWN);
  pinMode(BTN_F7, INPUT_PULLDOWN);

  pinMode(AXIS0, INPUT);
  pinMode(AXIS1, INPUT);
  pinMode(AXIS2, INPUT);

  pinMode(BAT_VT, INPUT);

  pinMode(LORA_CS, OUTPUT);
  pinMode(LORA_RXEN, OUTPUT);
  pinMode(LORA_TXEN, OUTPUT);
  pinMode(LORA_ISR, INPUT);

  Wire.begin();
  Wire.setClock(400000); 
  lcd.begin(Wire);  
  Serial.begin(115200);

  LoRa.setPins(LORA_CS, -2, LORA_ISR);// set CS, reset, IRQ pin

  lcd.setBacklight(lcdBrightness, lcdBrightness, lcdBrightness);
  lcd.setContrast(5); 
  lcd.disableSplash();  
  lcd.disableSystemMessages();

  digitalWrite(LORA_RXEN, HIGH);

  
  delay(200);
  lcd.clear(); 
  lcd.println("Wolfen.Tech @ 2022");
  lcd.println("v0.2 03/22");

   readBattery();
  lcd.println("Batt: " + String(batteryVoltage) + " " + String(batteryFraction*100.0));  

   
   // LoRa.setSignalBandwidth(125E3);
   // LoRa.setCodingRate4(3);
   // LoRa.setSpreadingFactor(7);
   // LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);


  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("[Setup] Radio Failure");
    lcd.println("Radio: FAIL");  
    while (true);                       // if failed, do nothing
  }

  lcd.println("Radio: OK"); 
  delay(1200);
  lcd.clear(); 
  LCDRefreshFlag = true;


}

void loop() {
   btn_f1 = digitalRead(BTN_F1);
   btn_f2 = digitalRead(BTN_F2);
   btn_f3 = digitalRead(BTN_F3);
   btn_f4 = digitalRead(BTN_F4);
   btn_c1 = digitalRead(BTN_F5);
   btn_c2 = digitalRead(BTN_F6);
   btn_c3 = digitalRead(BTN_F7);
   btn_j1 = digitalRead(BTN_JS);

   axis_x = (float)analogRead(AXIS0) * (1.65 / 2045.0);
   axis_y = (float)analogRead(AXIS1) * (1.65 / 2045.0);
   axis_z = (float)analogRead(AXIS2) * (1.65 / 2045.0);
   
      
   
   
   
   broadcastData();
   LoRa.receive();
   updateButtonStates();   
   LCDTask();         
   // onReceive(LoRa.parsePacket());
}

void updateButtonStates() 
{
 
 if (btn_f1 != btn_l_f1 && btn_f1 == LOW) {
    boostMode = !boostMode;
    Serial.println("[BoostMode]" + String(boostMode));    
 }

 if (btn_f2 != btn_l_f2 && btn_f2 == LOW) {    
    lcdBrightness += 50;
    if (lcdBrightness >= 300) {
       lcdBrightness = 0;
    }

    lcd.setBacklight(lcdBrightness, lcdBrightness, lcdBrightness);
 }

   
   
   btn_l_f1 = btn_f1;
   btn_l_f2 = btn_f2;
   btn_l_f3 = btn_f3;
   btn_l_f4 = btn_f4;
   btn_l_c1 = btn_c1;
   btn_l_c2 = btn_c2;
   btn_l_c3 = btn_c3;
   btn_l_j1 = btn_j1;

}

void LCDTask() {  
   if (LCDRefreshFlag == true) {
      lcdRefreshLineTimeStamp = millis();  
      
         if (lcdRefreshLineTurn == 0){
            readBattery();            
            String boostMessage = (boostMode == true) ? "BST" : "NRM";
            String message0 = String(batteryVoltage) + " [" + boostMessage + "] [" + TXStatus +"] " + String(txLatency);   
            if (message0 != message0Last) {
               lcd.setCursor(0,0);
               lcd.println(message0);  
               message0Last = message0;          
            }
            lcdRefreshLineTurn++;         
         } else if (lcdRefreshLineTurn == 1) {
            String message1 = "F" + String(btn_f1) + " " + String(btn_f2) + " " + String(btn_f3) + " " + String(btn_f4) + " C" + String(btn_c1) + " " + String(btn_c2) + " " + String(btn_c3) + " J" + String(btn_j1);
            if (message1 != message1Last) {
               lcd.setCursor(0,1);
               lcd.println(message1);
               message1Last = message1;
            } 
            lcdRefreshLineTurn++;
         } else if (lcdRefreshLineTurn == 2) {
            String message2 = "X:" + String(axis_x) + "v    Y:" + String(axis_y) + "v";
            if (message2 != message2Last) {
               lcd.setCursor(0,2);
               lcd.println(message2);
               message2Last = message2;
            }
            lcdRefreshLineTurn++;
         } else if (lcdRefreshLineTurn == 3) {
            String message3 = barDisplay(axis_z);
            if (message3 != message3Last) {
               lcd.setCursor(0,3);
               lcd.println(message3);
               message2Last = message3;
            }
            lcdRefreshLineTurn=0;
         }
      TXStatus = "NA";         
      LCDRefreshFlag = false;
   }   
}



void readBattery() {
  float rawValue = analogRead(BAT_VT);
   batteryVoltage = (rawValue / 4095.0) * 2 * 1.1 * 3.3; // calculate voltage level
   batteryFraction = batteryVoltage / 4.2;  
}

void onReceive(int packetSize) {
   if (packetSize<=0) {      
      return;
   }

  TXStatus = "[RX]";
  LCDRefreshFlag = true;
  lcd.setBacklight(255,0,0);

  String incoming = "";

  while (LoRa.available()) {
    incoming += LoRa.read();
  }

    Serial.println("[RX]: >" + incoming + "<");
}


void broadcastData() {
   bcastData.x = axis_x;
   bcastData.y = axis_y;
   bcastData.z = axis_z;
   bcastData.b1 = btn_c1;
   bcastData.b2 = btn_c2;
   bcastData.b3 = btn_c3;
   bcastData.bj = btn_j1;

   
  sendDataToCore(bcastData);   
}

void sendDataToCore(bcast_message message) { 
  char buffer[25];
  snprintf(buffer, sizeof(buffer),"%.1f,%.1f,%.1f,%i%i%i%i-", message.x, message.y, message.z, (uint8_t)message.b1, (uint8_t)message.b2, (uint8_t)message.b3,(uint8_t)message.bj);
  uint8_t bufferSize = strlen(buffer);
   
   
   // onReceive(LoRa.parsePacket());

   if (LoRa.beginPacket() == true) {
      digitalWrite(LORA_TXEN, LOW);
      digitalWrite(LORA_RXEN, LOW);

      if (boostMode == true) {         
         digitalWrite(LORA_TXEN, HIGH);
      }
      LoRa.print(buffer);  
      LoRa.print(bufferSize);
      LoRa.endPacket(true);
      digitalWrite(LORA_TXEN, LOW);                 
      TXStatus = "TX";     
      txLatency = millis() - lastTXTime; 
      Serial.println("[TX]: >" + String(buffer) + String(bufferSize) + "< (" + (txLatency) + ")");
      lastTXTime = millis();      
      failedCounter = 0; 
      LCDRefreshFlag = true;      

      if (boostMode == true) {
         digitalWrite(LORA_TXEN, LOW);  
         digitalWrite(LORA_RXEN, HIGH);
      }      
            


   } else {              
      if (failedCounter > 180 && failedCounter < 182) {
         TXStatus = "ERR";
         LCDRefreshFlag = true;
         failedCounter = 183; 
                     
      } else {
         failedCounter++;
      }   
   }  
  
}



String barDisplay(float volts) {
   int position = (int)(6.0 * volts);
   
   if (position >= 18) { position = 18; }

   String output = "";
   for (int i = 0; i <= 18; i++) {
      if (i == position) {
         output = output + "!";
      } else {
         output = output + " ";
      }

      
   }

   return output;
}
