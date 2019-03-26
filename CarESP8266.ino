#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#define WifiSSID "Car"
#define WifiPASS "987654321"

#define USE_SERIAL Serial
#define LEFT_1 4
#define LEFT_2 5
#define LEFT_EN 13
#define RIGHT_1 16
#define RIGHT_2 12
#define RIGHT_EN 14

int ADCvalue =0;

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void Set_Speed(String MOTOR, int SPEED) {
    int dir = 0;
    if(SPEED == 512) {
        digitalWrite(LEFT_1, LOW);
        digitalWrite(LEFT_2, LOW);
        digitalWrite(RIGHT_1, LOW);
        digitalWrite(RIGHT_2, LOW);
        return;
    }
    else if (SPEED > 512) {
      dir = 1;
    }
    SPEED = map(SPEED,0,1023,-700,700);
    if(SPEED <0){
      SPEED = SPEED* (-1);
    }
    SPEED+=300;

    if(MOTOR == "vertical") {
        USE_SERIAL.printf("vertical - Speed: %u Direction %u \n", SPEED, dir);
        if(!dir){
              analogWrite(LEFT_EN, SPEED);
              analogWrite(RIGHT_EN, SPEED);

              digitalWrite(LEFT_1, HIGH);//back
              digitalWrite(LEFT_2, LOW);
              digitalWrite(RIGHT_1, HIGH);
              digitalWrite(RIGHT_2, LOW);
        }
        else{
              analogWrite(LEFT_EN, SPEED);
              analogWrite(RIGHT_EN, SPEED);
              digitalWrite(LEFT_1, LOW);
              digitalWrite(LEFT_2, HIGH);
              digitalWrite(RIGHT_1, LOW);
              digitalWrite(RIGHT_2, HIGH);
        }
    } else if(MOTOR == "horizontal") {  
        USE_SERIAL.printf("horizontal - Speed: %u Direction %u \n", SPEED, dir);
        if(!dir){   
              analogWrite(LEFT_EN, SPEED);
              analogWrite(RIGHT_EN, SPEED);
              digitalWrite(LEFT_1, LOW);
              digitalWrite(LEFT_2, HIGH);
              digitalWrite(RIGHT_1, HIGH);
              digitalWrite(RIGHT_2, LOW);
        }
        else{
              analogWrite(LEFT_EN, SPEED);
              analogWrite(RIGHT_EN, SPEED);
              digitalWrite(LEFT_1, HIGH);
              digitalWrite(LEFT_2, LOW);
              digitalWrite(RIGHT_1, LOW);
              digitalWrite(RIGHT_2, HIGH);
        }
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
            String strpayload = (char*)payload;

            if(int location = strpayload.indexOf(':')) {
                String MOTOR = strpayload.substring(0, location);
                String SPEED = strpayload.substring(location+1);

                if(MOTOR == "stop")
                {
                  digitalWrite(LEFT_1, LOW);
                  digitalWrite(LEFT_2, LOW);
                  digitalWrite(RIGHT_1, LOW);
                  digitalWrite(RIGHT_2, LOW);
                }
                else if (MOTOR == "left")
                {
                  MOTOR = "horizontal";
                  SPEED = "250";
                int intSPEED = SPEED.toInt();
                Set_Speed(MOTOR, intSPEED);                  
                }
                else if (MOTOR == "right")
                {
                  MOTOR = "horizontal";
                  SPEED = "750";
                int intSPEED = SPEED.toInt();
                Set_Speed(MOTOR, intSPEED);                  
                }
                else if (MOTOR == "front")
                {
                  MOTOR = "vertical";
                  SPEED = "750";
                int intSPEED = SPEED.toInt();
                Set_Speed(MOTOR, intSPEED);                  
                }
                else if (MOTOR == "back")
                {
                  MOTOR = "vertical";
                  SPEED = "250";
                int intSPEED = SPEED.toInt();
                Set_Speed(MOTOR, intSPEED);                  
                }                
                else
                {
                int intSPEED = SPEED.toInt();
                
                
                Set_Speed(MOTOR, intSPEED);
                }
            }
            break;
    }

}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    pinMode(LEFT_1, OUTPUT);
    pinMode(LEFT_2, OUTPUT);
    pinMode(LEFT_EN, OUTPUT);
    pinMode(RIGHT_1, OUTPUT);
    pinMode(RIGHT_2, OUTPUT);
    pinMode(RIGHT_EN, OUTPUT);

WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  IPAddress localIp(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  
  WiFi.softAPConfig(localIp, gateway, subnet);
  WiFi.softAP(WifiSSID,WifiPASS);

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("robot")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // handle index
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", "<html><head><title>Controller</title><script type='text/javascript'>function send(n) {var o = parseInt(document.getElementById(n).value);console.log(n + ': ' + o.toString()),connection.send(n + ':' + o.toString())}var connection = new WebSocket('ws://' + location.hostname + ':81/',['arduino']);connection.onopen = function() {connection.send('#Connect ' + new Date)},connection.onerror = function(n) {console.log('WebSocket Error ', n)},connection.onmessage = function(n) {console.log('WebSocket Message ', n.data)},document.addEventListener('DOMContentLoaded', function(n) {var o = document.getElementById('vertical')  , e = document.getElementById('horizontal')  , t = document.getElementById('stop')  , c = document.getElementById('front')  , v = document.getElementById('back')  , b = document.getElementById('left')  , n = document.getElementById('right');t.onclick = function() {send('stop')},c.onmousedown = function(n) {send('front')},c.onmouseup = function(n) {o.value = 512,send('vertical'),o.onmousemove = null},v.onmousedown = function(n) {send('back')},v.onmouseup = function(n) {o.value = 512,send('vertical'),o.onmousemove = null},b.onmousedown = function(n) {send('left')},b.onmouseup = function(n) {e.value = 512,send('horizontal'),e.onmousemove = null},n.onmousedown = function(n) {send('right')},n.onmouseup = function(n) {e.value = 512,send('horizontal'),e.onmousemove = null},c.ontouchstart = function(n) {e.ontouchmove = function() {send('front')}},c.ontouchend = function(n) {o.value = 512,send('vertical'),o.ontouchmove = null},v.ontouchstart = function(n) {o.ontouchmove = function() {send('back')}},v.ontouchend = function(n) {o.value = 512,send('vertical'),o.ontouchmove = null},b.ontouchstart = function(n) {e.ontouchmove = function() {send('left')}},b.ontouchend = function(n) {e.value = 512,send('horizontal'),e.ontouchmove = null},n.ontouchstart = function(n) {e.ontouchmove = function() {send('right')}},n.ontouchend = function(n) {e.value = 512,send('horizontal'),e.ontouchmove = null},o.onmousedown = function(n) {o.onmousemove = function() {send('vertical')}},o.onmouseup = function(n) {o.value = 512,send('vertical'),o.onmousemove = null},e.onmousedown = function(n) {e.onmousemove = function() {send('horizontal')}},e.onmouseup = function(n) {e.value = 512,send('horizontal'),e.onmousemove = null},o.ontouchstart = function(n) {o.ontouchmove = function() {send('vertical')}},o.ontouchend = function(n) {o.value = 512,send('vertical'),o.ontouchmove = null},e.ontouchstart = function(n) {e.ontouchmove = function() {send('horizontal')}},e.ontouchend = function(n) {e.value = 512,send('horizontal'),e.ontouchmove = null}});</script></head><body><input id='vertical' type='range' min='0' max='1023' step='1' value='512' orient='vertical' style='writing-mode: bt-lr;-webkit-appearance: slider-vertical; display: inline; float: left; height: 18vh; width: 25%; zoom: 5'/><input id='horizontal' type='range' min='0' max='1023' step='1' value='512' orient='horizontal' style='writing-mode: bt-lr;-webkit-appearance: slider-horizontal; display: inline; float: right; height: 18vh; width: 25%; zoom: 5'/><input id='front' type='button' value='FRONT' style='margin-left: 50px'/><br><input id='left' type='button' value='LEFT'/><input id='stop' type='button' value='STOP'/><input id='right' type='button' value='RIGHT'/><br><input id='back' type='button' value='BACK' style='margin-left: 50px'/>   <h2 id='adc'></h2>     <script type='text/javascript'>window.onload = function(){setInterval(function(){  var xmlHttp = new XMLHttpRequest();xmlHttp.onreadystatechange = function() {  if (xmlHttp.readyState == 4 && xmlHttp.status == 200)   document.getElementById('adc').innerText = xmlHttp.responseText +'%';};xmlHttp.open('GET', '/adc', true);xmlHttp.send(null);}, 3000);};</script></body></html>");
    });

    server.on("/adc",[](){
      server.send(200,"text/html", String(ADCvalue));
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void loop() {
    ADCvalue = analogRead(A0);
    ADCvalue = map(ADCvalue,815,1023,0,100);
    delay(300);
    webSocket.loop();
    server.handleClient();
}
