#include <SoftwareSerial.h>

//NOTE use Arduino 1.5.5 to complile - wifi libs not compatable with my later versions
#include <plotly_streaming_wifi.h>

/**
 */
#include "WiFly.h"
#include <math.h> 
#include "Credentials.h"
// dump debug to serial 9600 O
#define DEBUG_TO_SERIAL 0
#define ASCII_START_TEXT 2
#define ASCII_END_TEXT 3
#define ROB_WS_MAX_STRING_DATA_LENGTH 100
int newlineCount = 0;
boolean printWebResponse = false;
int serialBufferLength = 0;

boolean started = true;
unsigned long lastRequestMillis = 0;

short pos = 0; // position in read serialBuffer
char serialBuffer[ROB_WS_MAX_STRING_DATA_LENGTH+1];
char inByte = 0;
volatile boolean waitingForResponse = false;
boolean haveConnected = false;
boolean handShakeSucessful = false;
// Sign up to plotly here: https://plot.ly
// View your API key and streamtokens here: https://plot.ly/settings
#define nTraces 6
//up to 6 Traces only
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
SoftwareSerial masterMaga(6,7);

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
    if (!haveConnected && inByte == '*') {
      haveConnected = true;     
      continue;
    } 

    if (!handShakeSucessful && haveConnected && inByte == '\n') {
      handShakeSucessful = true;
      continue;
    }
    if (!handShakeSucessful) {
      continue;
    }

  
    // add to our read serialBuffer
    serialBuffer[pos] = inByte;
    //   Serial.println(inByte); 
    pos++;
    //Serial.println(inByte);
    if (inByte == '\n' || pos==ROB_WS_MAX_STRING_DATA_LENGTH-1) //end of max field length
    {
      serialBuffer[pos-1] = 0; // delimit
      Serial.print("REQUEST: ");
      Serial.println(serialBuffer);
      processMastersRequest(); 
      serialBuffer[0] = '\0';
      pos = 0;      
    }
 
  }
}

boolean startedPlotting = false;

void processStartPloting() {
   if (startedPlotting) {
    masterMaga.println("E|Started plotting already!");\
    return;
   }
   char username[25];
   char password[25];
   char filename[31];
   int tokenCount;
   
    getValue(serialBuffer, '|', 1).toCharArray(username, 24);
    getValue(serialBuffer, '|', 2).toCharArray(password, 24);
    getValue(serialBuffer, '|', 3).toCharArray(filename, 30);
    boolean isOverWrite = (getValue(serialBuffer, '|', 4).toInt()==1);\
    int maxpoints = getValue(serialBuffer, '|', 5).toInt();
    tokenCount = getValue(serialBuffer, '|', 6).toInt();
    if (tokenCount>nTraces) {
      tokenCount=nTraces;
    }
    
    
    Serial.print("token count: ");
    Serial.println(tokenCount);
    char buf[11];
    String newTokens[nTraces];
  
    for(int i=0; i<tokenCount; i++) {      
      newTokens[i] = getValue(serialBuffer, '|', 7+i);
     // getValue(serialBuffer, '|', 6+i).c_str(buf, 11);
      //strcpy(tokens[i], getValue(serialBuffer, '|', 6+i).c_str());
      //tokens[i] = buf;
       Serial.println(newTokens[i]);
      // strcpy(tmpTokens[i], buf);
    }

    graph = new plotly(username, password, newTokens, filename, tokenCount, client);
    if (isOverWrite) graph->fileopt="overwrite";
    else graph->fileopt= "extend"; //overwrite"; // See the "Usage" section in https://github.com/plotly/arduino-api for details
    graph->maxpoints = maxpoints;
    bool success;
    success = graph->init();
    if(!success){
      while(!success){
        delay(1000);
        success = graph->init();
        //masterMaga.print("E|PLOTY ERROR");
        
      }
    }
    graph->openStream();
    startedPlotting = true;
    delay(500);
    masterMaga.println("OK|");  
    Serial.println("OK sent to mega");  
}

void processPlot() {
  if (!startedPlotting) {
    masterMaga.println("E|Cannot plot");
    return;
  }
   char token[11];
   char charType[3];
   char buf[25];
     
   getValue(serialBuffer, '|', 1).toCharArray(token, 11);
   getValue(serialBuffer, '|', 2).toCharArray(charType, 3);
   
   if (charType[0]=='I') {
     int i =getValue(serialBuffer, '|', 3).toInt();
     graph->plot(millis(),  i, token);
   } else {
     //float
     getValue(serialBuffer, '|', 3).toCharArray(buf, 24);
     float f = atof(buf);
     graph->plot(millis(),  f, token);
   }
   masterMaga.println("OK|");   
   Serial.println("OK sent to mega");  
}

void processMastersRequest() {
  char recordType = serialBuffer[0];
  switch(recordType) {
  case 'S' : // Start ploting a chart USERNAME|PASSWORD|FILENAME|IS OVER WRITE|TOKEN COUNT|[TOKENS|...]
    processStartPloting();
    break;
  case 'P' : // plot value to chart  TOKEN|CHAR TYPE|VALUE
    processPlot();
    break;
  }
}


void setup() {
  
  Serial.begin(9600);
  delay(60);
  masterMaga.begin(9600);

  WiFly.begin();

  while (!WiFly.join(ssid, passphrase)) {
    // Hang on failure.
    if (DEBUG_TO_SERIAL==1) {
      Serial.println("Association failed.");
    }       
    masterMaga.println("E|WIFI ERROR");
    delay(100);
  }
  
  masterMaga.print("**");
  delay(32);
  masterMaga.print("********\n"); 
 
}

void loop() {
  processMasterSerial();
}



