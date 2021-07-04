#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#include <JPEGCamera.h>

#include "Arduino.h"
#include "LittleFS.h"

#define CHUNK_SIZE 8192

//Create an instance of the camera
JPEGCamera camera;
//Create a character array to store the cameras response to commands
char response[CHUNK_SIZE];
//Count is used to store the number of characters in the response string.
unsigned int count=0;
//Size will be set to the size of the jpeg image.
int size=0;
//This will keep track of the data address being read from the camera
int address=0;
//eof is a flag for the sketch to determine when the end of a file is detected
//while reading the file data from the camera.
int eof=0;

// Replace with your network credentials
const char* ssid = "free-ax3000";
const char* password = "12348765";

// Initialize Telegram BOT
String BOTtoken = "1792141648:AAHnPMGVEXPGLtjW1Yx8qXLju0aiSj_Mq4c";  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
String CHAT_ID = "398870354";

File jpgFile;

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//#define DEBUG
#ifdef DEBUG
void printResponseFromCamera()
{
  Serial1.print("Response count - ");
  Serial1.print(count);
  Serial1.println("");
  for(int i = 0; i < count; i++)
  {
    Serial1.print(response[i], HEX);
    Serial1.print(" ");
  }
  Serial1.println("");
}
#else
void printResponseFromCamera()
{
}
#endif

byte getNextByte()
{
  return jpgFile.read();
}

bool isMoreDataAvailable()
{
  return jpgFile.available();
}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial1.println("handleNewMessages");
  Serial1.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial1.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/capture to take photo \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    
    if (text == "/capture") {
      bot.sendMessage(chat_id, "Start taking photo", "");
      //Take a picture
      memset(&response, 0, sizeof(response));
      count=camera.takePicture(response);
      //Print the response to the 'TAKE_PICTURE' command.
      printResponseFromCamera();
      Serial.flush();
      
      //Get the size of the picture
      memset(&response, 0, sizeof(response));
      count = camera.getSize(response, &size);    
      printResponseFromCamera();
      //Print the size
      Serial1.print("Size: ");
      Serial1.println(size);

      jpgFile = LittleFS.open(F("/j1.jpg"), "w");
 
      if (jpgFile){
        Serial1.println("Write file content!");
        address = 0;
        eof = 0;
        while(address < size)
        {
            //Read the data starting at the current address.
            Serial1.println("Read data");
            memset(&response, 0, sizeof(response));
            count=camera.readData(response, address, CHUNK_SIZE);
            printResponseFromCamera();
            //Store all of the data that we read to the SD card
            for(int i=0; i<count; i++){
                //Check the response for the eof indicator (0xFF, 0xD9). If we find it, set the eof flag
                if((response[i] == (char)0xD9) && (response[i-1]==(char)0xFF))
                {
                  eof=1;
                }
                //Save the data to the SD card
                jpgFile.write(response[i]);
                //If we found the eof character, get out of this loop and stop reading data
                if(eof==1)
                {
                  break;
                }
            }
            //Increment the current address by the number of bytes we read
            address+=count;
            Serial1.print("address - ");
            Serial1.println(address);
            Serial1.print("count - ");
            Serial1.println(count);
            //Make sure we stop reading data if the eof flag is set.
            if(eof==1)
            {
              Serial1.println("EOF file");
              break;
            }
        }
        jpgFile.close();
      }else{
        Serial1.println("Problem on create file!");
      }
      while(Serial.available())
        Serial1.write(Serial.read());
      Serial.flush(); //clear serial buffer
      Serial1.println("Done.");

      count = camera.stopPictures(response);
      printResponseFromCamera();
      
      Serial1.println("Send photo");
      jpgFile = LittleFS.open(F("/j1.jpg"), "r");
      Serial1.print("Size - ");
      Serial1.println(jpgFile.size());
      
      String sent = bot.sendPhotoByBinary(chat_id, "image/jpeg", jpgFile.size(),
                                        isMoreDataAvailable,
                                        getNextByte, nullptr, nullptr);
      jpgFile.close();
      LittleFS.remove(F("/j1.jpg"));
      if (sent)
      {
        Serial1.println("Was successfully sent");
      }
      else
      {
        Serial1.println("Was not sent!");
      }
    }
  }
}

void getFsInformation()
{
  // To format all space in LittleFS
  // LittleFS.format()
    
  // Get all information of your LittleFS
  FSInfo fs_info;
  LittleFS.info(fs_info);
 
  Serial1.println("File sistem info.");
 
  Serial1.print("Total space:      ");
  Serial1.print(fs_info.totalBytes);
  Serial1.println("byte");
 
  Serial1.print("Total space used: ");
  Serial1.print(fs_info.usedBytes);
  Serial1.println("byte");
 
  Serial1.print("Block size:       ");
  Serial1.print(fs_info.blockSize);
  Serial1.println("byte");
 
  Serial1.print("Page size:        ");
  Serial1.print(fs_info.totalBytes);
  Serial1.println("byte");
 
  Serial1.print("Max open files:   ");
  Serial1.println(fs_info.maxOpenFiles);
 
  Serial1.print("Max path lenght:  ");
  Serial1.println(fs_info.maxPathLength);
 
  Serial1.println();
 
  // Open dir folder
  Dir dir = LittleFS.openDir("/");
  // Cycle all the content
  while (dir.next()) {
    // get filename
    Serial1.print(dir.fileName());
    Serial1.print(" - ");
    // If element have a size display It else write 0
    if(dir.fileSize()) {
      File f = dir.openFile("r");
      Serial1.println(f.size());
      f.close();
    }else{
      Serial1.println("0");
    }
  }
}

void setup() {
  // configure debug port (it supports only TX)
  Serial1.begin(115200);
  
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial1.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial1.println(WiFi.localIP());

  Serial1.println(F("Inizializing FS..."));
  if (LittleFS.begin()){
    Serial1.println(F("done."));
  }else{
    Serial1.println(F("fail!"));
  }
  // Get File system information
  getFsInformation();

  camera.begin();
  //Reset the camera
  Serial1.println("Reset camera");
  memset(&response, 0, sizeof(response));
  count=camera.reset(response);
  printResponseFromCamera();
  delay(5000);
  while(Serial.available())
    Serial1.write(Serial.read());
  Serial1.println("");
  Serial.flush(); //clear serial buffer
    
  /*Serial1.println("Set compression ratio");
  memset(&response, 0, sizeof(response));
  count=camera.compressionRatio(response);
  Serial1.print("Response count - ");
  Serial1.println(count);
  Serial1.print("Response - ");
  for(int i = 0; i < count; i++)
  {
    Serial1.print(response[i], HEX);
    Serial1.print(" ");
  }
  Serial1.println("");
  delay(3000);*/
  /*Serial1.println("Set image size");
  camera.imageSize(response);*/
/*
  Serial1.println("Set baudrate");
  count=camera.setBaudrate(response);
  Serial1.print("Response count - ");
  Serial1.println(count);
  Serial1.print("Response - ");
  for(int i = 0; i < count; i++)
  {
    Serial1.print(response[i], HEX);
    Serial1.print(" ");
  }
  Serial1.println("");
  delay(2000);
  Serial.end();
  delay(500);
  Serial.begin(11500);
  //Serial.swap(); //GPIO15 (TX) and GPIO13 (RX)
  Serial.flush(); //clear serial buffer*/
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial1.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
