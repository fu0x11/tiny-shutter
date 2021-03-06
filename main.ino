/* main.ino
 *
 * francesco urbani
 *
 */

#include <ESP8266WiFi.h>

/* define of commands used to trigger the shutter. 
 * depending on whether I use the optocoupler or I connect the camera directly to GPIO0, these two needs to be swapped.
 */
#define TRIGGER 1   /* TRIGGER = 0, shoot occurs if GPIO0 is attached to the camera. */
                    /* TRIGGER = 1, shoot occurs if the optocoupler is involved. */
#define HALT    0

#define INDEX  "<!DOCTYPE html> <head> <html lang='en'> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1.0'/> <link rel='icon' type='image/png' href='img/dslr_favicon.png'/> <title>Tiny shutter</title> <style> h1 { font-family: arial; color: blue; text-align: center; font-size: 20px; } button { font-family: arial; box-shadow: 0 12px 36px 0 rgba(0,0,0,0.24), 0 20px 50px 0 rgba(0,0,0,0.19); width: 100%; height: 80px; /* height of a single button*/ border: none; color: #d8d8d8; text-align: center; font-size: 30px; background-color: #3b02ff; } #shootButton { background-color: #0a0aae; } #intervallometerButton { background-color: #3fc5ff; } #copy { font-size: 8px; margin-top: 1cm; font-family: monospace; text-align: center; } </style> </head> <!-- <h1>Camera remote</h1> --> <!-- fork me on GitHub ribbon --> <!-- <a href='https://github.com/fu0x11/camera-shutter-esp8266' target='_blank'><img style='position: absolute; top: 0; right: 0; border: 0; height: 2.6cm; width: 2.6cm' src='img/forkme.png' data-canonical-src=''></a> --> <body> <p><a href=index.html><button id='shootButton'>SINGLE SHOOT</button></a></p> <!-- /shoot --> <!-- index.html --> <p><a href=index.html><button>DELAYED SHOOT</button></a></p> <!-- /delshoot --> <!-- index.html --> <p><a href=index.html><button>BURST</button></a></p> <!-- /burst --> <!-- index.html --> <p><a href=index.html><button>DELAYED BURST</button></a></p> <!-- /delburst --> <!-- index.html --> <p><a href=interv.html><button id='intervallometerButton'>INTERVALLOMETER</button></a></p> <!-- /interv --> <!-- innterv.html --> </body> <!-- copy --> <div id='copy'> &copy francesco urbani </div> <!-- end copy --> </html> "
#define INTERV "<!DOCTYPE html> <head> <html lang='en'> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1.0'/> <link rel='icon' type='image/png' href='img/dslr_favicon.png'/> <title>Tiny shutter</title> <style> h1 { font-family: arial; color: #ff0000; text-align: center; font-size: 20px; } button { font-family: arial; box-shadow: 0 12px 36px 0 rgba(0,0,0,0.24), 0 20px 50px 0 rgba(0,0,0,0.19); width: 100%; height: 464px; /* result of (height of a button)*(number of buttons)+(default button distance)*(number of distances) */ /* 80 * 5 + 16 * 4 */ border: none; color: #d8d8d8; text-align: center; font-size: 80px; background-color: #00DD00; } #offButton { background-color: #ff0000; } #copy { font-size: 8px; margin-top: 1cm; font-family: monospace; text-align: center; } </style> </head> <!-- fork me on GitHub ribbon --> <!-- <a href='https://github.com/fu0x11/camera-shutter-esp8266' target='_blank'><img style='position: absolute; top: 0; right: 0; border: 0; height: 2.6cm; width: 2.6cm' src='img/forkme.png' data-canonical-src=''></a> --> <!-- <h1>Camera remote</h1> --> <body> <p><a href=index.html><button id='offButton'>OFF</button></a></p> <!-- /off --> <!-- index.html --> <!-- copy --> <div id='copy'> &copy francesco urbani </div> <!-- end copy --> </body> </html> "

const int SHOOT_PIN = 2;    /* LED hooked up on GPIO 0 */
const int SHOOTING_TIME = 500;  // ms. the time that the shutter button is being pressed.
                                // in that time the camera needs to focus and shoot.
const int INTERV_TIME = 5000-SHOOTING_TIME;   // intervallometer time = 5 seconds.

WiFiServer server(80);
void blinkLED();

void setup() {
  initHardware();
  setupWiFi();
  server.begin();
}

void loop() {

  WiFiClient client = server.available();   /* Check if a client has connected */
  if (!client) return;

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Prepare the response. Start with the common header and change the rest according to the request:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";


  // Match the request
  int val = -1;   /* We'll use 'val' to keep track of both the request type (read/set) and value if set. */

  if (req.indexOf("/shoot") != -1)
    val = 0;
  else if (req.indexOf("/off") != -1)
    val = 1;
  else if (req.indexOf("/inter") != -1)
    val = 2;
  else if (req.indexOf("/delshoot") != -1)
    val = 3;
  else if (req.indexOf("/burst") != -1)
    val = 4;
  else if (req.indexOf("/delburst") != -1)
    val = 5;
  else    /* Otherwise the request is invalid. */
    val = 1; 

  client.flush();

  switch (val) {
    case 0: // shoot
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      shoot();
      break;
    case 1: // off
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      digitalWrite(SHOOT_PIN, HALT);
      break;
    case 2: // intervallometer
      /* interv.html */
      s += INTERV;
      client.flush();
      client.print(s);
      intervallometer();
      break;
    case 3: // delayed_shoot
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      digitalWrite(SHOOT_PIN, HALT);
      delay(2000);  /* ms of delay before shooting */
      shoot();
      break;
    case 4: // burst. Warning: the camera needs to be set on burst mode too!
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      digitalWrite(SHOOT_PIN, TRIGGER);
      delay(3000);
      digitalWrite(SHOOT_PIN, HALT);
      break;
    case 5: // delayed burst. Warning: the camera needs to be set on burst mode too!
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      delay(2000);  // ms of delay before burst.
      digitalWrite(SHOOT_PIN, TRIGGER);
      delay(3000);
      digitalWrite(SHOOT_PIN, HALT);
      break;
    default:
      /* index.html */
      s += INDEX;
      client.flush();
      client.print(s);
      val = 1;
      digitalWrite(SHOOT_PIN, HALT);
  }
 
  // client.print(s); /* Send the response to the client */
                      /* displays the actual webpage */

  Serial.println("Client disonnected");  /* The client will actually be disconnected when the function returns and 'client' object is detroyed */
}




void setupWiFi() {
  WiFi.mode(WIFI_AP);   /* configure esp8266 as access point */
  const char AP_NameChar[] = "";
  const char WiFiAPPSK[]   = "";
  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}

void initHardware() {
  // Serial.begin(115200);
  pinMode(SHOOT_PIN, OUTPUT);
  digitalWrite(SHOOT_PIN, HALT);
  return;
}

void shoot(){
	digitalWrite(SHOOT_PIN, HALT);
  digitalWrite(SHOOT_PIN, TRIGGER);
  delay(SHOOTING_TIME);
  digitalWrite(SHOOT_PIN, HALT);
  return;
}

void intervallometer(){
  while(1){
    digitalWrite(SHOOT_PIN, HALT);
    digitalWrite(SHOOT_PIN, TRIGGER);
    delay(SHOOTING_TIME);
    digitalWrite(SHOOT_PIN, HALT);
    delay(INTERV_TIME);
  }
  return;
}
