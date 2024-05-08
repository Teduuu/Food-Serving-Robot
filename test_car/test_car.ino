#include <esp32-hal-dac.h>
#include <Wire.h>
#include "PID_v1.h"
//#include "Timer.h"
#define INTERVAL_MESSAGE 1
#define INTERVAL_MESSAGE1 1000
#include <WiFi.h>
#include <HTTPClient.h>
#define TIMEOUT 250

//const String SERVER_IP = "http://172.20.10.4:5000" ;
const String SERVER_IP = "http://172.20.10.2:5000" ;
//const String SERVER_IP = "http://192.168.43.216:5000" ;
int position_now = 0;
char car_status = 's';
char car_status1 = 's';
int data_send = 5;
int go_to_seat = 0;
int theta_go;
int theta_go_pass;
/* create client object*/
HTTPClient http;


/*set led pin*/
const int freq = 5000;
const int resolution = 8;
/*[HTTP]debug*/

int INC=19; //left_hall_signal
int IND=27; //right_hall_signal
int INA=18; //left wheel
int INB=14; //right wheel

int L_Hall_signal_correct; //左輪正確的訊號
int L_Hall_signal_correct_last; //左輪上一次正確的訊號
int L_Hall_signal; //左輪霍爾訊號
int L_Hall_signal_last; //左輪上一次的霍爾訊號

int R_Hall_signal_correct; //右輪正確的訊號
int R_Hall_signal_correct_last; //右輪上一次正確的訊號
int R_Hall_signal; //右輪霍爾訊號
int R_Hall_signal_last; //右輪上一次的霍爾訊號

int number; //1毫秒內，如果上一次的霍爾訊號如果等於這一次的霍爾訊號，則number+1
double L_count; //左輪計算0.1秒內有幾次上升振緣
double L_count1; //左輪上一次上升振緣次數
double R_count; //右輪計算0.1秒內有幾次上升振緣
double R_count1; //右輪上一次上升振緣次數
double L_freq; //左輪頻率
double R_freq; //右輪頻率
unsigned long time_now=0;
unsigned long time_now1=0;
int L_counter(); //左輪算1毫秒波是否跳動三次
int R_counter(); //右輪算1毫秒波是否跳動三次
void L_Getfreq(); //求左輪頻率
void R_Getfreq(); //求右輪頻率

double kp_r=0.28,ki_r=6.9,kd_r=0.0;
double input;
double output;
double setpoint;
PID mypid_right(&input, &output, &setpoint, kp_r, ki_r, kd_r, DIRECT);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WIFI_INIT();
  
  Wire.begin();
  delay(50);
  
  pinMode(INA,OUTPUT);
  pinMode(INB,OUTPUT);
  pinMode(INC,INPUT);
  pinMode(IND,INPUT);
  
  //analog output(PWM) test  
  ledcSetup(0, 5000, 8);
  ledcAttachPin(INA, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(INB, 1);
}

void loop() {
  //===============================================Server===========================================================// 
  String response = server_gather_http("where_to_go"); //get data from server 
  if(response.toInt() >= 0){
    go_to_seat = response.toInt();
    }
      Serial.print("response from server is ：");
      Serial.println(go_to_seat);
    
      if(car_status == 't')  //去廚房
      { 
        data_send = 1;  
      }
      else if (car_status == 'k') //廚房等餐
      { 
        data_send = 2;
      }
      else if (car_status == 'r') //去餐桌`
      { 
        data_send = 3;
      }
      else if (car_status == 'f') //餐桌服務
      { 
        data_send = 4;
      }
      else if  (car_status == 's') //門口待命
      { 
        data_send = 5;
      }
      else if  (car_status == 'b') //門口待命
      { 
        data_send = 6;
      }
      else if (car_status == 'd')
      { 
        data_send = 7;
      }
      else
      {
        data_send = 8;
      }
      String rqs = server_update_http(data_send);//send data to server
      if (rqs == "finished") 
      {
        Serial.println("done");
      }

   
      
      
  
 //==========================================Server===================================================================//
  
  //====================傳輸==============================================================================================//  
    
  

  Wire.beginTransmission(8); // transmit to device #8
  Wire.write(go_to_seat);  // sends one byte 
  Wire.endTransmission();    // stop transmitting
  

  Wire.requestFrom(8,2);    // request 1 bytes from peripheral device #8
  while (Wire.available()) { // peripheral may send less than requested
    car_status = Wire.read();
    theta_go = Wire.read();
  }
    if(theta_go>180)
    {
      theta_go = theta_go - 256;
    }
      Serial.print("car_status is :");
      Serial.println(car_status);
      Serial.print("theta_go");
      Serial.println(theta_go);
   if( theta_go==90 && (theta_go - theta_go_pass) >= 60 )
   {
    theta_go = theta_go_pass;
   }
   else if( theta_go==-90 && (theta_go - theta_go_pass)<= -60 )
   {
    theta_go = theta_go_pass;
   }
   else
   {
    theta_go_pass = theta_go;
   }
      
//======================傳輸============================================================================================//

//=============================直走轉彎============================================================================================//
  if(car_status=='r' || car_status=='t' || car_status=='b' || car_status=='d'){
     if(car_status != car_status1){
       ledcWrite(0, 255);//避免電流太低無法啟動
       ledcWrite(1, 255);//避免電流太低無法啟動
       delay(20);
       ledcWrite(0, 100);
       ledcWrite(1, 100);       
     }
     else{
       if(theta_go<=5 && theta_go>=-5){
         ledcWrite(0, 102);
         ledcWrite(1, 100);
       }
       else if(theta_go<60 && theta_go>5){
         ledcWrite(0, 105);
         ledcWrite(1, 100);         
       }
       else if(theta_go>-60 && theta_go<-5){
         ledcWrite(0, 100);
         ledcWrite(1, 104);
       }
       else if(theta_go>=60)
       {
        ledcWrite(0, 106);
        ledcWrite(1, 100);      
       }
       else if(theta_go<=-60)
       {
        ledcWrite(0, 100);
        ledcWrite(1, 106);

       }
     }
  }
  else
  {
    if(car_status1=='r' || car_status1=='t' || car_status1=='b'){
      /*ledcWrite(0, 0);
      ledcWrite(1, 0);
      delay(500);
      ledcWrite(0, 255);
      delay(20);
      ledcWrite(0, 140);
      ledcWrite(1, 0);
      delay(1500);*/
    }
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }

  car_status1=car_status;
  
//=============================直走轉彎====================================================================================//
  delay(180); 

}
//PID
int L_counter(){
  number=0;
  time_now = millis();
  while(millis()<time_now+INTERVAL_MESSAGE){
    L_Hall_signal=digitalRead(INC);
    if(L_Hall_signal==HIGH && L_Hall_signal_last==HIGH)
      number=number+1;
    else
      number=number;
    L_Hall_signal_last=L_Hall_signal;
  }
  if(number>=3){
    return 1;
  }
  else
    return 0;
}

int R_counter(){
  number=0;
  time_now = millis();
  while(millis()<time_now+INTERVAL_MESSAGE){
    R_Hall_signal=digitalRead(IND);
    if(R_Hall_signal==HIGH && R_Hall_signal_last==HIGH)
      number=number+1;
    else
      number=number;
    R_Hall_signal_last=R_Hall_signal;
  }
  if(number>=3){
    return 1;
  }
  else
    return 0;
}

void L_Getfreq(){
  time_now1 = millis();
  while(millis()<time_now1+INTERVAL_MESSAGE1){
    L_Hall_signal_correct=L_counter();
    if(L_Hall_signal_correct_last==0 && L_Hall_signal_correct==1){
      L_count++;
    }
    L_Hall_signal_correct_last=L_Hall_signal_correct;
  }
  L_freq=(L_count-L_count1);
  L_count1=L_count;
}


void R_Getfreq(){
  time_now1 = millis();
  while(millis()<time_now1+INTERVAL_MESSAGE1){
    R_Hall_signal_correct=R_counter();
    if(R_Hall_signal_correct_last==0 && R_Hall_signal_correct==1){
      R_count++;
    }
    R_Hall_signal_correct_last=R_Hall_signal_correct;
  }
  R_freq=(R_count-R_count1);
  R_count1=R_count;
}

void PID_control(){
  L_Getfreq();
  setpoint=L_freq;
  mypid_right.Compute();
  ledcWrite(1, 105+output);
  R_Getfreq();
  input=R_freq+output;
  Serial.println("R_freq:");
  Serial.println(R_freq);
  Serial.println("L_freq");
  Serial.println(L_freq);
  delay(500);
}

//===============================================WiFi Server======================================================================//
void WIFI_INIT(){

  /* give 60 seconds to connect WIFI, else faild*/
  int i = 60;
  //WiFi.begin("Tedu", "0921612932");
  WiFi.begin("Weiwei", "1234567890");
  //WiFi.begin("xiang", "88888888");
  while (WiFi.status() != WL_CONNECTED && i) {
    delay(1000); i--;
    Serial.println("[Wi-Fi] Connecting to WiFi...");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Wi-Fi] Connection faild.");
  }else{
    Serial.println("[Wi-Fi] Connection successed.");
    /* first get request, begin and add header*/
    String HTTP = "/esp32?mode=SSID,SSID=";
    HTTP = SERVER_IP + HTTP + WiFi.SSID();
    http.begin(HTTP);
    http.addHeader("Content-Type", "text/plain");  
    /* set client request timeout and sent SSID to server*/
    http.setTimeout(TIMEOUT);
    http.setConnectTimeout(TIMEOUT);
    int httpCode = http.GET(); //Send the request
    /* handle the server response*/
    String payload;
    if (httpCode > 0) {
      /* if get correct respone */
      payload = http.getString();   //Get the request response payload
      Serial.println("[Wi-Fi] server response: " + payload);             //Print the response payload
    }else{
      /* cannot send SSID to server, but keep going */
      Serial.println("[Wi-Fi] ERROR of response when first connecting to server");
      Serial.println("[Wi-Fi] keep going");
      if(httpCode == -1){ /* HTTPC_ERROR_CONNECTION_REFUSED */
        Serial.println("[Wi-Fi] Server offline, closing :( ");
        http.end();
      }
    }
  }
}

/* 
 * only use get request and don't disconnect with server 
 * which call "http persistence" 
 * 
 * control = analog control signal to motor
 * speed = encoder RMP to esp32
 * sensor = mass center from arduino to esp32 */
/* send data to sever */
String server_update_http(int hellofromesp){
  /* connect and add header */
  String HTTP="/esp32?mode=update";
  HTTP = SERVER_IP + HTTP + 
    "&hellofromesp=" + String(hellofromesp);
    
  http.begin(HTTP);
  http.addHeader("Content-Type", "text/plain"); 
  /* set client request timeout*/
  http.setTimeout(TIMEOUT);
  http.setConnectTimeout(TIMEOUT);
  /* sent GET request */ 
  int httpCode = http.GET(); //Send the request
  /* handle the server response*/
  if (httpCode > 0) { //Check the returning code
    /* if respone is normal, return respone as string */
    String payload = http.getString();   //Get the request response payload
    return payload;
  }else{
    /* if respone is bad, return httpCode as String */
    /* blink the led(error)*/
    return String(httpCode); 
  }
  http.end();
}

/* get data from sever */
String server_gather_http(char* data_get){
  /* connect and add header */
  String HTTP="/esp32?mode=gather&which=";
  HTTP = SERVER_IP + HTTP + data_get;
  http.begin(HTTP);
  http.addHeader("Content-Type", "text/plain"); 
  /* set client request timeout*/
  http.setTimeout(TIMEOUT);
  http.setConnectTimeout(TIMEOUT);
  /* sent GET request */ 
  int httpCode = http.GET(); //Send the request
  /* handle the server response*/
  if (httpCode > 0) { //Check the returning code
    /* if respone is normal, return respone as string */
    String payload = http.getString();   //Get the request response payload
    return payload;
  }else{
    /* if respone is bad, return httpCode as String */
    return String(httpCode); 
  }
  http.end();
}

//======================================================WiFi Server===========================================================//
