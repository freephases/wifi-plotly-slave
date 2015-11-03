/**
* Wifi Slave -
*
* Watch for memory usage as wifi card and maths lbs take up a lot at run time
*/
#include <SoftwareSerial.h>

//NOTE use Arduino 1.5.5 to complile - wifi libs not compatable with my later versions
#include <plotly_streaming_wifi.h>

/**
 */
#include "WiFly.h"
#include <math.h>
// dump debug to serial 9600 O
#define DEBUG_TO_SERIAL 0
#define ROB_WS_MAX_STRING_DATA_LENGTH 120

int serialBufferLength = 0;

boolean started = true;
unsigned long lastRequestMillis = 0;

short pos = 0; // position in read serialBuffer
char serialBuffer[ROB_WS_MAX_STRING_DATA_LENGTH + 1];
char inByte = 0;

boolean haveConnectedToWifi = false;

// Sign up to plotly here: https://plot.ly
// View your API key and streamtokens here: https://plot.ly/settings
// note max nTraces is 5 due to memeory at run time
#define nTraces 5
//up to x Traces only
// View your tokens here: https://plot.ly/settings
// Supply as many tokens as data traces
// e.g. if you want to ploty A0 and A1 vs time, supply two tokens
//char *tokens[nTraces] = {"dfghrjsy27","d7ehjduegs"};
// arguments: username, api key, streaming token, filename
// HTTP client
Client *client;

//plotly graph = plotly("username", "password", tokens, "your_filename", nTraces, client);
plotly *graph;
// *****************************
SoftwareSerial masterMaga(6, 7);

/**
 * Process data sent vy master
 */
void processMasterSerial()
{
  // send data only when you receive data:
  while (masterMaga.available() > 0)
  {

    // read the incoming byte:
    inByte = masterMaga.read();

    if (inByte == '\r') continue;

    // add to our read serialBuffer
    serialBuffer[pos] = inByte;
    //   Serial.println(inByte);
    pos++;


    //Serial.println(inByte);
    if (inByte == '\n' || pos == ROB_WS_MAX_STRING_DATA_LENGTH - 1) //end of max field length
    {
      serialBuffer[pos - 1] = 0; // delimit
      if (DEBUG_TO_SERIAL == 1) {
        Serial.print("REQUEST: ");
        Serial.println(serialBuffer);
      }
      processMastersRequest();
      serialBuffer[0] = '\0';
      pos = 0;
    }

  }
}

char wifiSSID[30];
char wifiPassword[20];
    
void beginWiFiSession() {
  if (!haveConnectedToWifi) {

   

    getValue(serialBuffer, '|', 1).toCharArray(wifiSSID, 30);
    getValue(serialBuffer, '|', 2).toCharArray(wifiPassword, 20);

    if (DEBUG_TO_SERIAL == 1) {
      Serial.println(wifiSSID);
      Serial.println(wifiPassword);
    }
    if (!WiFly.join(wifiSSID, wifiPassword)) {
      // Hang on failure.
      if (DEBUG_TO_SERIAL == 1) {
        Serial.println("Association failed, check SSID and password are correct");
      }
      masterMaga.println("E|No connection");
    }
    else {
      haveConnectedToWifi = true;
      masterMaga.println("OK|");
    }
  } else {
    masterMaga.println("OK|Already conencted");
  }
}

boolean startedPlotting = false;

void processStartPloting() {
  if (startedPlotting) {
    masterMaga.println("E|Started plotting already!"); 
    if (DEBUG_TO_SERIAL == 1) {
      Serial.println("E|Started plotting already!"); 
    }
    return;
  }
  char username[24];
  char password[24];
  char filename[30];
  int tokenCount;

  getValue(serialBuffer, '|', 1).toCharArray(username, 24);
  getValue(serialBuffer, '|', 2).toCharArray(password, 24);
  getValue(serialBuffer, '|', 3).toCharArray(filename, 30);
  boolean isOverWrite = (getValue(serialBuffer, '|', 4).toInt() == 1); 
  int maxpoints = getValue(serialBuffer, '|', 5).toInt();
  tokenCount = getValue(serialBuffer, '|', 6).toInt();
  if (tokenCount > nTraces) {
    tokenCount = nTraces;
  }


  //Serial.print("token count: ");
  //Serial.println(tokenCount);
  char buf[11];
  String newTokens[nTraces];

  for (int i = 0; i < tokenCount; i++) {
    newTokens[i] = getValue(serialBuffer, '|', 7 + i);
  }

  graph = new plotly(username, password, newTokens, filename, tokenCount, client);
  if (isOverWrite) graph->fileopt = "overwrite";
  else graph->fileopt = "extend"; //overwrite"; // See the "Usage" section in https://github.com/plotly/arduino-api for details
  graph->maxpoints = maxpoints;
  bool success = graph->init();
  if (!success) {
    masterMaga.print("E|PLOTY ERROR");
    while (!success) {      
      delay(1000);
      success = graph->init();
      
    }
  }
  graph->openStream();
  startedPlotting = true;
 
  masterMaga.println("OK|");
  delay(500); 
  if (DEBUG_TO_SERIAL == 1) {
    Serial.println("OK sent to mega");
  }
}

  char token[11];
  char charType[3];
  float fVal = 0.000;

void processPlot() {
  if (!startedPlotting) {
    masterMaga.println("E|Cannot plot");
    if (DEBUG_TO_SERIAL == 1) {
      Serial.println("E|Cannot plot"); 
    }
    return;
  }


  getValue(serialBuffer, '|', 1).toCharArray(token, 11);
  getValue(serialBuffer, '|', 2).toCharArray(charType, 3);

  if (charType[0] == 'I') {
    int i =getValue(serialBuffer, '|', 3).toInt();
    graph->plot(millis(), i, token);
  } else {
   fVal = getValue(serialBuffer, '|', 3).toFloat()+0.0000; //+0.0000 fixes bug with float string conversions later on
    graph->plot(millis(),  fVal, token);
  }
  masterMaga.println("OK|");
  if (DEBUG_TO_SERIAL == 1) {
    Serial.println("OK sent to mega");
  }
  //masterMaga.println("OK|");
}

void processMastersRequest() {
  char recordType = serialBuffer[0];
  switch (recordType) {
    case 'S' : // Start ploting a chart USERNAME|PASSWORD|FILENAME|IS OVER WRITE|TOKEN COUNT|[TOKENS|...]
      if (haveConnectedToWifi) {
        processStartPloting();
      }
      break;
    case 'P' : // plot value to chart  TOKEN|CHAR TYPE|VALUE
      if (haveConnectedToWifi) {
        processPlot();
      }
      break;

    case 'B' : // begin WiFi session
      beginWiFiSession();
      break;

    case '*' : // connected ok, send back ok
      masterMaga.print("*\n");
      delay(100);
      break;
  }
}


void setup() {

  Serial.begin(9600);
  masterMaga.begin(9600);

  WiFly.begin();


  /*while (!WiFly.join(ssid, passphrase)) {
     // Hang on failure.
     if (DEBUG_TO_SERIAL==1) {
       Serial.println("Association failed.");
     }
     //masterMaga.println("E|WIFI ERROR");
    // delay(100);
   }*/

  //  masterMaga.print("*\n");
  //   delay(500);
  //  masterMaga.print("***\n");
  //  delay(300);
  /// masterMaga.print("****\n"); //should know we are here by now we hope, depands if it was lookign i guess DOH! ;)


}

void loop() {
  processMasterSerial();
}



