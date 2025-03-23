         /////////////////////////////////////////////  
        //    Vegetables and Fruits Ripeness       // 
       //    Detection by Color w/ TensorFlow     //
      //           -----------------             //
     //          (Arduino Nano 33 IoT)          //           
    //             by Kutluhan Aktar           // 
   //                                         //
  /////////////////////////////////////////////

// 
// Collate spectral color data of varying fruits and vegetables and interpret this data set w/ a neural network model to predict ripening stages. 
//
// For more information:
// https://www.theamplituhedron.com/projects/Vegetables_and_Fruits_Ripeness_Detection_by_Color_w_TensorFlow/
//
//
// Connections
// Arduino Nano 33 IoT :           
//                                AS7341 11-Channel Spectral Color Sensor
// 3.3V --------------------------- +
// GND  --------------------------- -
// A5   --------------------------- C
// A4   --------------------------- D
//                                10K Potentiometer
// A0   --------------------------- S
//                                Class Button_1 (6x6)
// D2   ---------------------------
//                                Class Button_2 (6x6)
// D3   ---------------------------
//                                Class Button_3 (6x6)
// D4   ---------------------------
//                                Class Button_4 (6x6)
// D5   ---------------------------
//                                5mm Green LED
// D6   ---------------------------
//                                5mm Red LED
// D7   ---------------------------


// Include required libraries:
#include <WiFiNINA.h>
#include "DFRobot_AS7341.h"

char ssid[] = "SSID";        // your network SSID (name)
char pass[] = "PASSWORD";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;              // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

// Enter the IPAddress of your Raspberry Pi.
IPAddress server(192, 168, 1, 20);

// Initialize the Ethernet client library
WiFiClient client; /* WiFiSSLClient client; */

// Define the AS7341 object.
DFRobot_AS7341 as7341;
// Define AS7341 data objects:
DFRobot_AS7341::sModeOneData_t data1;
DFRobot_AS7341::sModeTwoData_t data2;

// Define the potentiometer pin:
#define pot A0

// Define the class button pins:
#define class_1 2
#define class_2 3
#define class_3 4
#define class_4 5

// Define the control LED pins:
#define green 6
#define red 7

// Define the data holders:
int pot_val, class_1_val, class_2_val, class_3_val, class_4_val;

void setup(){
  
  Serial.begin(9600);

  // Class button settings:
  pinMode(class_1, INPUT_PULLUP);
  pinMode(class_2, INPUT_PULLUP);
  pinMode(class_3, INPUT_PULLUP);
  pinMode(class_4, INPUT_PULLUP);
  // Control LED settings:
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  
  // Detect if I2C can communicate properly 
  while (as7341.begin() != 0) {
    Serial.println("I2C init failed, please check if the wire connection is correct");
    delay(1000);
  }

  // Enable the built-in LED on the AS7341 sensor.
  as7341.enableLed(true);

  // Check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) { Serial.println("Connection Failed!"); while (true); }
  // Attempt to connect to the WiFi network:
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect to WiFi !!!");
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // Wait 10 seconds for connection:
    delay(10000);
  }

}
void loop(){

  read_controls();
  adjust_brightness();

  // Start spectrum measurement:
  // Channel mapping mode: 1.eF1F4ClearNIR
  as7341.startMeasure(as7341.eF1F4ClearNIR);
  // Read the value of sensor data channel 0~5, under eF1F4ClearNIR
  data1 = as7341.readSpectralDataOne();
  // Channel mapping mode: 2.eF5F8ClearNIR
  as7341.startMeasure(as7341.eF5F8ClearNIR);
  // Read the value of sensor data channel 0~5, under eF5F8ClearNIR
  data2 = as7341.readSpectralDataTwo();
  // Print data:
  Serial.print("F1(405-425nm): "); Serial.println(data1.ADF1);
  Serial.print("F2(435-455nm): "); Serial.println(data1.ADF2);
  Serial.print("F3(470-490nm): "); Serial.println(data1.ADF3);
  Serial.print("F4(505-525nm): "); Serial.println(data1.ADF4);
  Serial.print("F5(545-565nm): "); Serial.println(data2.ADF5);
  Serial.print("F6(580-600nm): "); Serial.println(data2.ADF6);
  Serial.print("F7(620-640nm): "); Serial.println(data2.ADF7);
  Serial.print("F8(670-690nm): "); Serial.println(data2.ADF8);
  // CLEAR and NIR:
  Serial.print("Clear_1: "); Serial.println(data1.ADCLEAR);
  Serial.print("NIR_1: "); Serial.println(data1.ADNIR);
  Serial.print("Clear_2: "); Serial.println(data2.ADCLEAR);
  Serial.print("NIR_2: "); Serial.println(data2.ADNIR);
  Serial.print("\n------------------------------\n");
  delay(1000);

  // Send data to the PHP web application (Vegetables_and_Fruits_Data_Logger) depending on the selected ripeness class [0 - 3]:
  if(!class_1_val) make_a_get_request("/Vegetables_and_Fruits_Data_Logger/", "0");
  if(!class_2_val) make_a_get_request("/Vegetables_and_Fruits_Data_Logger/", "1");
  if(!class_3_val) make_a_get_request("/Vegetables_and_Fruits_Data_Logger/", "2");
  if(!class_4_val) make_a_get_request("/Vegetables_and_Fruits_Data_Logger/", "3");

}

void read_controls(){
  // Potentiometer:
  pot_val = analogRead(pot);
  // Class Buttons:
  class_1_val = digitalRead(class_1);
  class_2_val = digitalRead(class_2);
  class_3_val = digitalRead(class_3);
  class_4_val = digitalRead(class_4);
}

void adjust_brightness(){
  // Set pin current to control brightness (1~20 corresponds to current 4mA,6mA,8mA,10mA,12mA,......,42mA)
  int brightness = map(pot_val, 0, 1023, 1, 21);
  Serial.print("\nBrightness: "); Serial.println(brightness);
  as7341.controlLed(brightness);
}

void make_a_get_request(String application, String _class){
  // Connect to the web application named Vegetables_and_Fruits_Data_Logger. Change '80' with '443' if you are using SSL connection.
  if(client.connect(server, 80)){
    // If successful:
    Serial.println("\n\nConnected to the server!");
    // Create the query string:
    String query = application + "?F1="+data1.ADF1+"&F2="+data1.ADF2+"&F3="+data1.ADF3+"&F4="+data1.ADF4+"&F5="+data2.ADF5+"&F6="+data2.ADF6+"&F7="+data2.ADF7+"&F8="+data2.ADF8+"&nir_1="+data1.ADNIR+"&nir_2="+data2.ADNIR+"&class="+_class;
    // Make an HTTP Get request:
    client.println("GET " + query + " HTTP/1.1");
    client.println("Host: 192.168.1.20");
    client.println("Connection: close");
    client.println();
  }else{
    Serial.println("Server Error!");
    digitalWrite(red, HIGH);
  }
  delay(2000); // Wait 2 seconds after connection...
  // If there are incoming bytes available, get the response from the web application.
  String response = "";
  while (client.available()) { char c = client.read(); response += c; }
  if(response != ""){
    Serial.println(response);
    Serial.println("\n");
    // Check whether the transferred data is inserted successfully or not:
    if(response.indexOf("Data Inserted Successfully!") > 0){
      digitalWrite(green, HIGH);
    }else{
      digitalWrite(red, HIGH);
    }
  }
  // Turn off LEDs:
  delay(3000);
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
}
