/* Arduino JPEGCamera Library
 * Copyright 2010 SparkFun Electronic
 * Written by Ryan Owens
*/

#include <avr/pgmspace.h>
#include "JPEGCamera.h"
#include "Arduino.h"

//Commands for the LinkSprite Serial JPEG Camera
const char GET_SIZE[5] = {0x56, 0x00, 0x34, 0x01, 0x00};
const char RESET_CAMERA[4] = {0x56, 0x00, 0x26, 0x00};
const char TAKE_PICTURE[5] = {0x56, 0x00, 0x36, 0x01, 0x00};
const char IMAGE_SIZE[9] = {0x56, 0x00, 0x31, 0x05, 0x04, 0x01, 0x00, 0x19, 0x00};
const char STOP_TAKING_PICS[5] = {0x56, 0x00, 0x36, 0x01, 0x03};
const char READ_DATA[8] = {0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00};
const char SET_BAUD[7] = {0x56, 0x00, 0x24, 0x03, 0x01, 0x0D, 0xA6};                         
const char COMPRESSION_RATIO[9] = {0x56, 0x00, 0x31, 0x05, 0x01, 0x01, 0x12, 0x04, 0x36}; 

JPEGCamera::JPEGCamera()
{
}

//Initialize the serial connection
void JPEGCamera::begin(void)
{
	//Camera baud rate is 38400
	Serial.begin(38400);
	Serial.swap(); //GPIO15 (TX) and GPIO13 (RX)
	Serial.flush(); //clear serial buffer
}

//Get the size of the image currently stored in the camera
//pre: response is an empty string. size is a pointer to an integer
//post: response string contains the entire camera response to the GET_SIZE command
//		size is set to the size of the image
//return: number of characters in the response string
//usage: length = camera.getSize(response, &img_size);
int JPEGCamera::getSize(char * response, int * size)
{
	int count=0;
	//Send the GET_SIZE command string to the camera
	count = sendCommand(GET_SIZE, response, 5);
	//Read 4 characters from the camera and add them to the response string
	for(int i=0; i<4; i++)
	{
		while(!Serial.available());
		response[count+i]=Serial.read();
	}
	
	//Set the number of characters to return
	count+=4;
	
	//The size is in the last 2 characters of the response.
	//Parse them and convert the characters to an integer
    *size = response[count-2]*256;
    *size += (int)response[count-1] & 0x00FF;	
	//Send the number of characters in the response back to the calling function
	return count;
}

//Reset the camera
//pre: response is an empty string
//post: response contains the cameras response to the reset command
//returns: number of characters in the response
//Usage: camera.reset(response);
int JPEGCamera::reset(char * response)
{
	return sendCommand(RESET_CAMERA, response, 4);
}

//Take a new picture
//pre: response is an empty string
//post: response contains the cameras response to the TAKE_PICTURE command
//returns: number of characters in the response
//Usage: camera.takePicture(response);
int JPEGCamera::takePicture(char * response)
{
	return sendCommand(TAKE_PICTURE, response, 5);
}

//Stop taking pictures
//pre: response is an empty string
//post: response contains the cameras response to the STOP_TAKING_PICS command
//returns: number of characters in the response
//Usage: camera.stopPictures(response);
int JPEGCamera::stopPictures(char * response)
{
	return sendCommand(STOP_TAKING_PICS, response, 5);
}

//Set image size
//pre: response is an empty string
//post: response contains the cameras response to the IMAGE_SIZE command
//returns: number of characters in the response
//Usage: camera.imageSize(response);
int JPEGCamera::imageSize(char * response)
{
	return sendCommand(IMAGE_SIZE, response, 9);
}

//Set baudrate
//pre: response is an empty string
//post: response contains the cameras response to the SET_BAUD command
//returns: number of characters in the response
//Usage: camera.setBaudrate(response);
int JPEGCamera::setBaudrate(char * response)
{
	return sendCommand(SET_BAUD, response, 7);
}

//Set compression ratio
//pre: response is an empty string
//post: response contains the cameras response to the COMPRESSION_RATIO command
//returns: number of characters in the response
//Usage: camera.compressionRation(response);
int JPEGCamera::compressionRatio(char * response)
{
	return sendCommand(COMPRESSION_RATIO, response, 9);
}

//Get the read_size bytes picture data from the camera
//pre: response is an empty string, address is the address of data to grab
//post: response contains the cameras response to the readData command, but the response header is parsed out.
//returns: number of characters in the response
//Usage: camera.readData(response, cur_address);
//NOTE: This function must be called repeatedly until all of the data is read
//See Sample program on how to get the entire image.
int JPEGCamera::readData(char * response, int address, int read_size)
{
	int count=0;

	//Flush out any data currently in the serial buffer
	Serial.flush();
	
	//Send the command to get read_size bytes of data from the current address
	for(int i=0; i<8; i++)Serial.write(READ_DATA[i]);
	Serial.write(address>>8);
	Serial.write(address);
	Serial.write(0x00);
	Serial.write(0x00);
	Serial.write(read_size>>8);
	Serial.write(read_size);
	Serial.write(0x00);
	Serial.write(0x0A);
	
	//Print the data to the serial port. Used for debugging.
	/*
	for(int i=0; i<8; i++)Serial.print(READ_DATA[i]);
	Serial.print(address>>8, BYTE);
	Serial.print(address, BYTE);
	Serial.print(0x00, BYTE);
	Serial.print(0x00, BYTE);
	Serial.print(read_size>>8, BYTE);
	Serial.print(read_size, BYTE);
	Serial.print(0x00, BYTE);
	Serial.print(0x0A, BYTE);	
	Serial.println();
	*/

	//Read the response header.
	for(int i=0; i<5; i++){
		while(!Serial.available());
		Serial.read();
	}
	
	//Now read the actual data and add it to the response string.
	count=0;
	while(count < read_size)
	{
		while(!Serial.available());
		*response++=Serial.read();
		count+=1;
	}
	
	//Return the number of characters in the response.
	return count;
}

//Send a command to the camera
//pre: cameraPort is a serial connection to the camera set to 3800 baud
//     command is a string with the command to be sent to the camera
//     response is an empty string
//	   length is the number of characters in the command
//post: response contains the camera response to the command
//return: the number of characters in the response string
//usage: None (private function)
int JPEGCamera::sendCommand(const char * command, char * response, int length)
{
	char c=0;
	int count=0;
	//Clear any data currently in the serial buffer
	Serial.flush();
	Serial1.print("Send command - ");
	for(int i=0; i<length; i++)
	{
		Serial1.print(command[i], HEX);
		Serial1.print(" ");
	}
	Serial1.println("");
	
	//Send each character in the command string to the camera through the camera serial port
	for(int i=0; i<length; i++){
		Serial.print(*command++);
	}
	
	while(!Serial.available());
	while(true)
	{
		if(Serial.available())
		{
			*response = Serial.read();
			Serial1.print(*response, HEX);
			Serial1.print(" ");
			if(*response == 0x76)
			{
				break;
			}
		}
	}
	Serial1.println("");
		
	*response++;
	count+=1;	
	//Get the response from the camera and add it to the response string.
	for(int i=1; i<length; i++)
	{
		while(!Serial.available());
		*response++=Serial.read();	
		count+=1;
	}
	
	//return the number of characters in the response string
	return count;
}
