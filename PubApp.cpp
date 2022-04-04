#include<iostream>
#include<stdio.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/i2c.h>
#include<linux/i2c-dev.h>
#include<iomanip>
#include<unistd.h>
#include <math.h>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"
#include <time.h>
#include <string>
#include <sstream>
#define CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"

using namespace std;
#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)
#define ADDRESS "tcp://192.168.222.70:1883"
#define CLIENTID "rpi1"
#define AUTHMETHOD "azaz"
#define AUTHTOKEN "mashwani10"
#define TOPIC "eee513/CPUTemp"
#define QOS 2
#define TIMEOUT 10000L
// The ADXL345 Resisters required for this example
#define DEVID       0x00
#define POWER_CTL   0x2D
#define DATA_FORMAT 0x31
#define DATAX0      0x32
#define DATAX1      0x33
#define DATAY0      0x34
#define DATAY1      0x35
#define DATAZ0      0x36
#define DATAZ1      0x37
#define BUFFER_SIZE 0x40
unsigned char dataBuffer[BUFFER_SIZE];

// Write a single value to a single register
int writeRegister(int file, unsigned char address, char value){
   unsigned char buffer[2];
   buffer[0] = address;
   buffer[1] = value;
   if (write(file, buffer, 2)!=2){
      cout << "Failed write to the device" << endl;
      return 1;
   }
   return 0;
}
float getCPUTemperature() { // get the CPU temperature
int cpuTemp; // store as an int
fstream fs;
fs.open(CPU_TEMP, fstream::in); // read from the file
fs >> cpuTemp;
fs.close();
return (((float)cpuTemp)/1000);
}
// Read the entire 40 registers into the buffer (10 reserved)
int readRegisters(int file){
   // Writing a 0x00 to the device sets the address back to
   //  0x00 for the coming block read
   writeRegister(file, 0x00, 0x00);
   if(read(file, dataBuffer, BUFFER_SIZE)!=BUFFER_SIZE){
      cout << "Failed to read in the full buffer." << endl;
      return 1;
   }
   if(dataBuffer[DEVID]!=0xE5){
      cout << "Problem detected! Device ID is wrong" << endl;
      return 1;
   }
   return 0;
}

// short is 16-bits in size on the Raspberry Pi
short combineValues(unsigned char msb, unsigned char lsb){
   //shift the msb right by 8 bits and OR with lsb
   return ((short)msb<<8)|(short)lsb;
}

int main(){
   int file;
   cout << "Starting the ADXL345 sensor application at ---  QoS 2" << endl;
   if((file=open("/dev/i2c-1", O_RDWR)) < 0){
      cout << "failed to open the bus" << endl;
      return 1;
   }
   if(ioctl(file, I2C_SLAVE, 0x53) < 0){
      cout << "Failed to connect to the sensor" << endl;
      return 1;
   }
   writeRegister(file, POWER_CTL, 0x08);
   //Setting mode to 00000000=0x00 for +/-2g 10-bit
   //Setting mode to 00001011=0x0B for +/-16g 13-bit
   writeRegister(file, DATA_FORMAT, 0x00);
   readRegisters(file);

   
   cout << "The Device ID is: " << HEX(dataBuffer[DEVID]) << endl;
   cout << "The POWER_CTL mode is: " << HEX(dataBuffer[POWER_CTL]) << endl;
   cout << "The DATA_FORMAT is: " << HEX(dataBuffer[DATA_FORMAT]) << endl;
   cout << dec << endl;   //reset back to decimal
   

 
   int count=0;
   MQTTClient client;
   while(count < 10){
      time_t now =time(0);
      tm *ltm = localtime(&now);
      int hour = ltm->tm_hour;
      int min = ltm->tm_min;
      int sec = ltm->tm_sec;
      int year= 1900+ltm->tm_year;
      int month= 1+ltm->tm_mon;
      int day= ltm->tm_mday;
      short x = combineValues(dataBuffer[DATAX1], dataBuffer[DATAX0]);
      short y = combineValues(dataBuffer[DATAY1], dataBuffer[DATAY0]);
      short z = combineValues(dataBuffer[DATAZ1], dataBuffer[DATAZ0]);
      float resolution = 1024.0f;
      float gravity_range = 4.0f;
      float factor= (gravity_range/resolution);
      float accXg = (x*factor);
      float accYg = (y*factor);
      float accZg = (z*factor);
      float accXSquared = accXg * accXg ;
      float accYSquared = accYg * accYg ;
      float accZSquared = accZg * accZg ;
      float pitch = 180 * atan(accXg/sqrt(accYSquared + accZSquared))/M_PI;
      float roll = 180 * atan(accYg/sqrt(accXSquared + accZSquared))/M_PI;
      //Use \r and flush to write the output on the same line
      cout << "Pitch = " <<pitch<< " Roll = "<<roll<<"         \r"<<flush;
      
      readRegisters(file);  //read the sensor again
      char str_payload[100]; // Set your max message size here
      
      MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
      MQTTClient_message pubmsg = MQTTClient_message_initializer;
      MQTTClient_deliveryToken token;
      MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
      MQTTClient_willOptions wills = MQTTClient_willOptions_initializer;
      wills.topicName = "eee513/CPUTemp";
      wills.message =  "Publisher is lost";
      wills.qos = QOS;
      opts.keepAliveInterval = 20;
      opts.cleansession = 1;
      opts.username = AUTHMETHOD;
      opts.password = AUTHTOKEN;
      opts.will = &wills;
      
      int rc;
      if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
      cout << "Failed to connect, return code " << rc << endl;
      return -1;
      }
      sprintf(str_payload, "{\"CPUTemp\": %.2f,\"Pitch\":%.2f, \"Roll\":%.2f, \"Time\": %d:%d:%d, \"Date\":%d-%d-%d}", getCPUTemperature(),pitch,roll,hour,min,sec,day,month,year);
      pubmsg.payload = str_payload;
      pubmsg.payloadlen = strlen(str_payload);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
      cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
      " seconds for publication of \n" << str_payload <<
      " \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
      rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
      cout << "Message with token " << (int)token << " delivered." << endl;
      usleep(1000000);
      
      count++;
      
      
   }
   
   
   MQTTClient_disconnect(client, 10000);
   cout<<"normal disconnect"<< endl;
   usleep(10000000);
   return 0;
}
