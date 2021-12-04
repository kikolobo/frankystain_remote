#include <Wire.h>
#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
#include "WiFi.h"
#include <esp_now.h>

//Const
static const uint8_t  AXIS0 =  A3;
static const uint8_t  AXIS1 =  A2;
static const uint8_t  AXIS2 =  A4;
static const uint8_t  BTN_JS =  A10;
static const uint8_t  BTN_F1 =  A12;
static const uint8_t  BTN_F2 =  A11;
static const uint8_t  BTN_F3 =  TX;
static const uint8_t  BTN_F4 =  21;
static const uint8_t  BTN_F5 =  A9;
static const uint8_t  BTN_F6 =  A7;
static const uint8_t  BTN_F7 =  A6;

//Vars
int btn_f1  = 0;
int btn_f2  = 0;
int btn_f3  = 0;
int btn_f4  = 0;
int btn_c1  = 0;
int btn_c2  = 0;
int btn_c3  = 0;
int btn_j1  = 0;
float axis_x  = 0;
float axis_y  = 0;
float axis_z  = 0;
bool LCDRefreshFlag = false; 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//Objects
SerLCD lcd; // Initialize the library with default I2C address 0x72


//BroadcastMSG
// REPLACE WITH YOUR RECEIVER MAC Address
// MY address: C4:DD:57:67:31:D0
uint8_t broadcastAddress[] = {0xC4, 0xDD, 0x57, 0x67, 0x31, 0xD0};

// Structure example to send data
typedef struct bcast_message {  
  float x;
  float y;
  float z;  
  bool b1;
  bool b2;
  bool b3;
  bool bj;  
} bcast_message;

// Create a struct_message called myData
bcast_message bcastData;

String TXStatus = "NA";
//Prototypes
void IRAM_ATTR onTime();
String barDisplay(float volts);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void broadcastData();


//Program
void setup() {
  Wire.begin();
  Wire.setClock(400000); 
  lcd.begin(Wire);
  WiFi.mode(WIFI_MODE_STA);

  
  Serial.begin(115200);

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


  lcd.setBacklight(100, 100, 100);
  lcd.setContrast(5); 
  lcd.disableSplash();  
  lcd.disableSystemMessages();

  
  delay(200);
  lcd.clear(); 
  lcd.println("Wolfen.Tech @ 2021");
  lcd.println("v0.1 08 01");
  lcd.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error: initializing ESP-NOW");    
  }
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Error: Failed to add peer");
    return;
  }

  delay(1200);
  lcd.clear(); 

  // Configure the Prescaler at 80 the quarter of the ESP32 is cadence at 80Mhz
   // 80000000 / 80 = 1000000 tics / seconde
   timer = timerBegin(0, 80, true);                
   timerAttachInterrupt(timer, &onTime, true);    
    
   // Sets an alarm to sound every second
   timerAlarmWrite(timer, 300000 , true);
   timerAlarmEnable(timer);

    
    
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
   

if (LCDRefreshFlag == true) {
    // lcd.setCursor(0, 1);

    
    String message0 = "      [Inputs]  " + TXStatus;
    String message1 = "F" + String(btn_f1) + " " + String(btn_f2) + " " + String(btn_f3) + " " + String(btn_f4) + " C" + String(btn_c1) + " " + String(btn_c2) + " " + String(btn_c3) + " J" + String(btn_j1);
    String message2 = "X:" + String(axis_x) + "v    Y:" + String(axis_y) + "v";
    TXStatus = "NA";
   //  String message3 = "Z:" + String(axis_z) + "v";

    String message3 = barDisplay(axis_z);

    lcd.setCursor(0,0);
    lcd.println(message0);
    lcd.println(message1);
    lcd.println(message2);
    lcd.print(message3);
    LCDRefreshFlag = false;
}

   broadcastData();
}

void broadcastData() {

   bcastData.x = axis_x;
   bcastData.y = axis_y;
   bcastData.z = axis_z;
   bcastData.b1 = btn_c1;
   bcastData.b2 = btn_c2;
   bcastData.b3 = btn_c3;
   bcastData.bj = btn_j1;

  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &bcastData, sizeof(bcastData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
    TXStatus = "TX";
  }
  else {
    Serial.println("Error sending the data");
    TXStatus = "ER";
  }
}

void IRAM_ATTR onTime() {
   portENTER_CRITICAL_ISR(&timerMux);
   LCDRefreshFlag = true;
   portEXIT_CRITICAL_ISR(&timerMux);
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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}