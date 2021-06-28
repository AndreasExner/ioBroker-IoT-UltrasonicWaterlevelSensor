#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define LED D4
#define TRIGGER D5
#define ECHO D6

const char* ssid     = "xxxx";
const char* password = "yyyy";

String baseURL_INT = "http://192.168.1.1:8087/getPlainValue/0_userdata.0.IoT.Watertank.Interval";
String baseURL_IP = "http://192.168.1.1:8087/set/0_userdata.0.IoT.Watertank.SensorIP?value=";
String baseURL_RST = "http://192.168.1.1:8087/set/0_userdata.0.IoT.Watertank.RebootFlag?value=";

String baseURL_DIST = "http://192.168.1.1:8087/set/0_userdata.0.IoT.Watertank.Distance?value=";

int interval = 300; 
int counter = 0;

long duration;
int distance;

void(* HWReset) (void) = 0;

//####################################################################
// Setup

void setup() {  
  
  Serial.begin(115200);  
  delay(100);
  pinMode(LED, OUTPUT);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

//---------------------------------------------------------------------
// Reset Distance Trigger

  digitalWrite(TRIGGER, LOW);
  
//---------------------------------------------------------------------
// WiFi Connect

  Serial.println('\n');
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(++i); Serial.print(' ');
    if (i > 20) {
      Serial.println("Connection failed!");  
      reboot_on_error();
    }
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  send_ip();
  send_rst();
  get_interval();
  send_distance();
}


//####################################################################
// Measure Distance

void get_distance() {
  
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  duration = pulseIn(ECHO, HIGH);
  
  distance = duration*0.034/2;
  
  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.println(" cm");
}


//####################################################################
// Send distance ioBroker

void send_distance() {

  int b = 0;

  do {
    get_distance();
    b++;
    if (b > 10) {
      Serial.println("Error getting the distance");
      break;
    }
    delay(100);
  }
  while (distance < 1);

  if (distance > 1) {
  
    HTTPClient http;
  
    String sendURL = baseURL_DIST + String(distance);
    http.begin(sendURL);
    http.GET();
    String payload = http.getString();
    Serial.println(payload);
    
    http.end();
  }
} 


//####################################################################
// Reboot on Error

void reboot_on_error() {

  Serial.println('\n');
  Serial.println("Performing reboot in 30 seconds....");

  delay(30000);

  HWReset();
}


//####################################################################
// get Interval from iobroker

void get_interval() {

  int intervall_current = interval;
  
  HTTPClient http;
  
  http.begin(baseURL_INT);
  http.GET();
  
  int interval_new = http.getString().toInt();

  if (intervall_current != interval_new) {
    interval = interval_new;
    Serial.println("New Interval = " + String(interval));  
  }
  
  http.end();
}


//####################################################################
// Send local IP to ioBroker

void send_ip() {
  
  HTTPClient http;
  
  Serial.println(WiFi.localIP());

  String sendURL = baseURL_IP + WiFi.localIP().toString();

  http.begin(sendURL);
  int httpcode = http.GET();
  String payload = http.getString();
  Serial.println("HTTP Return = " + String(httpcode));
  Serial.println(payload);
  http.end();

  if (httpcode != 200) {
    Serial.println("Error sending data to iobroker!");
    reboot_on_error();    
  }
} 

//####################################################################
// Send RST ioBroker

void send_rst() {
  
  HTTPClient http;
  
  Serial.println("Send RST");
  String sendURL = baseURL_RST + "true";

  http.begin(sendURL);
  http.GET();
  String payload = http.getString();
  Serial.println(payload);
  http.end();

  delay(100);

  sendURL = baseURL_RST + "false";
  http.begin(sendURL);
  http.GET();
  payload = http.getString();
  Serial.println(payload);
  http.end();
}  


//####################################################################
// Loop
  
void loop() {

  int c = 0;

  if (counter >= interval) {
    send_ip();
    get_interval();
    send_distance();
    counter = 0;
  }
  Serial.println(counter);
  ++counter;

//---------------------------------------------------------------------
// Blink LED

  while (c < 10) {
    delay(50);
    ++c;
  }

  digitalWrite(LED, HIGH);        
  c = 0;

  while (c < 10) {
    delay(50);
    ++c;
  }

  digitalWrite(LED, LOW);         
  c = 0;
}
