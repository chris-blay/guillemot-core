#include "motorController.h"
#include "SubMotorController/MotorCurrentMsg.h"
#include "time.h"
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <string>

using namespace std;


void printMessageMismatchError() {
}

ros::NodeHandle* n;

void MotorControllerHandler::print(string error) {
	bool init = false;
	if(!init) {
		errorOut = n->advertise<std_msgs::String>("/Error_Log", 100);
		init = true;
	}
	printf("ErrorHandler: %s\n", error.c_str());
	std_msgs::String msg;
	msg.data = error;
	errorOut.publish(msg);
}

MotorControllerHandler::MotorControllerHandler(ros::NodeHandle* nh, const char* Port)
	: serialPort(Port) {
		n = nh;

	awaitingResponse = false;
	bufIndex = 0;
	currentMessage.type = NO_MESSAGE;
	gettimeofday(&lastQRCurTime, NULL);
	gettimeofday(&lastQLCurTime, NULL);
	gettimeofday(&lastQVoltTime, NULL);
	gettimeofday(&lastMotorTime, NULL);
	rightSpeed = leftSpeed = rightTargetSpeed = leftTargetSpeed = 0;
	MaxStep = 20;
	name = Port;
	try {
		serialPort.Open(BAUD, SerialPort::CHAR_SIZE_8, SerialPort::PARITY_NONE, SerialPort::STOP_BITS_1, SerialPort::FLOW_CONTROL_NONE);
	} catch (...) {
		char temp[1000];
		sprintf(temp, "%s error: Failed during initialization\n", name.c_str());
		print(string(temp));
		bufIndex = 0;
	}
	//motorStatus = n->advertise<SubMotorController::MotorDataMessage>("/Motor_Data", 10);
	motorCurrent = n->advertise<SubMotorController::MotorCurrentMsg>("/Motor_Current", 100);
}

void MotorControllerHandler::sendMessage(Message m) {
	currentMessage = m;
	transmit();
}

int filter(int speed) {
	if(speed >= 256)
		speed = 255;
	if(speed <= -256)
		speed = -255;
	speed = -speed;
	return speed;
}

void MotorControllerHandler::setMotorSpeed(int right, int left) {
	rightTargetSpeed = filter(right);
	leftTargetSpeed = filter(left);
//	printf("setting target speeds to %d %d\n", rightTargetSpeed, leftTargetSpeed);
}

Message createMessageFromSpeed(int rightSpeed, int leftSpeed) {
	Message msg;
	msg.type = MOTOR_TYPE;

	if(leftSpeed > 0) {
		msg.DataC[0] = leftSpeed;
		msg.DataC[1] = 0;
	} else {
		int rev = -leftSpeed;
		msg.DataC[0] = 0;
		msg.DataC[1] = rev;
	}
	if(rightSpeed > 0) {
		msg.DataC[2] = rightSpeed;
		msg.DataC[3] = 0;
	} else {
		int rev = -rightSpeed;
		msg.DataC[2] = 0;
		msg.DataC[3] = rev;
	}

	return msg;
}

void MotorControllerHandler::transmit() {
	if(currentMessage.type == NO_MESSAGE)
		return;

	gettimeofday(&lastSendTime, NULL);
	awaitingResponse = true;

	if(!serialPort.IsOpen()) {
		try {
			serialPort.Open(BAUD, SerialPort::CHAR_SIZE_8, SerialPort::PARITY_NONE, SerialPort::STOP_BITS_1, SerialPort::FLOW_CONTROL_NONE);
		} catch (...) {
			char temp[1000];
			sprintf(temp, "%s error: Unable to open port\n", name.c_str());
			print(string(temp));
		}
	}
	try {
		serialPort.WriteByte('S');
		serialPort.WriteByte(currentMessage.type);
		for(int i = 0; i < 4; i++) {
			serialPort.WriteByte(currentMessage.DataC[i]);
		}
		serialPort.WriteByte('E');
		awaitingResponse = true;
	} catch (SerialPort::NotOpen serError){
		char temp[1000];
		sprintf(temp, "%s error: Port not open - %s\n", name.c_str(), serError.what());
		print(string(temp));
	} catch (std::runtime_error runError){
        char temp[1000];
		sprintf(temp, "%s error: Unable to send message - %s\n", name.c_str(), runError.what());
		print(string(temp));
	}catch (...){
        char temp[1000];
		sprintf(temp, "%s error: Unable to send message - Unknown exception\n", name.c_str());
		print(string(temp));
	}
}

void MotorControllerHandler::processResponse() {
	if(buffer[0] != 'S' || buffer[6] != 'E') {
		//Misaligned data? throw out bytes until you get it to align correctly
		printf("Misaligned data: ");
		for(int i = 0; i < 7; i++)
			printf("\'%c\' (%x)", buffer[i], buffer[i]);
		printf("\n");
		for(int i = 0; i < 6; i++) {
			buffer[i] = buffer[i+1];
		}
		bufIndex--;
		return;
	}

	bufIndex = 0;
	Message response;
	response.type = buffer[1];
	for(int i = 0; i < 4; i++) {
		response.DataC[i] = buffer[i+2];
	}

	//printf("got response %c %c %x %x %x %x %c\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
	switch (response.type) {
		case ERROR_TYPE:
			char temp[1000];
			sprintf(temp, "%s error from controller: %c%c%c%c\n", name.c_str(), buffer[2], buffer[3], buffer[4], buffer[5]);
			print(string(temp));

			awaitingResponse = false;
			break;
		case MOTOR_RESPONSE_TYPE:
			if(currentMessage.type != MOTOR_TYPE) {
				printMessageMismatchError();
				break;
			}
			if(response.DataC[0])
				leftSpeed = response.DataC[0];
			else
				leftSpeed = -response.DataC[1];

			if(response.DataC[2])
				rightSpeed = response.DataC[2];
			else
				rightSpeed = -response.DataC[3];
			currentMessage.type = NO_MESSAGE;
			awaitingResponse = false;
			break;
		case CURRENT_RESPONSE_TYPE:
			if(currentMessage.type != CURRENT_TYPE) {
				printMessageMismatchError();
				break;
			}

			if(currentMessage.DataC[0] == 'L')
			{
				LeftCurrent = response.DataF;
				SubMotorController::MotorCurrentMsg msg;
				msg.motorName = name;
				msg.motorPosition = "Left";
				msg.motorCurrent = LeftCurrent;

				motorCurrent.publish(msg);
			}
			else
			{
				RightCurrent = response.DataF;
				SubMotorController::MotorCurrentMsg msg;
				msg.motorName = name;
				msg.motorPosition = "Right";
				msg.motorCurrent = RightCurrent;

				motorCurrent.publish(msg);
			}

			currentMessage.type = NO_MESSAGE;
			awaitingResponse = false;
			break;
		case VOLTAGE_RESPONSE_TYPE:
			if(currentMessage.type != VOLTAGE_TYPE) {
				printMessageMismatchError();
				break;
			}
			Voltage = response.DataF;
			currentMessage.type = NO_MESSAGE;
			awaitingResponse = false;
			break;
		default:
			printf("Unrecognized response type: %c\n", response.type);
	}
}

void MotorControllerHandler::receive() {
	if(serialPort.IsOpen() && awaitingResponse) {
		try {
			while(serialPort.IsDataAvailable()) {
				unsigned char data = serialPort.ReadByte();
//				printf("received byte \'%c\'\n", data);
				while(bufIndex == 7) {
					processResponse();
				}
				buffer[bufIndex++] = data;
				if(bufIndex == 7) {
					processResponse();
				}
			}
		} catch (...) {
			char temp[1000];
			sprintf(temp, "%s error: While attempting to read data\n", name.c_str());
			print(string(temp));
		}
	}
}

int getMilliSecsBetween(timeval& start, timeval& end) {
	int millis = (end.tv_sec - start.tv_sec) * 1000;
	millis += (end.tv_usec - start.tv_usec) / 1000;
	return millis;
}

bool MotorControllerHandler::TransmitTimeout() {
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int elsaped = getMilliSecsBetween(lastSendTime, curTime);
	if(elsaped > RESEND_TIMEOUT)
		return true;
	return false;
}

void MotorControllerHandler::CheckQuery() {
	if(awaitingResponse)
		return;
	timeval curtime;
	gettimeofday(&curtime, NULL);
	int elsaped = getMilliSecsBetween(lastQRCurTime, curtime);
	if(elsaped > CURRENT_PERIOD) {
		Message query;
		query.type = CURRENT_TYPE;
		query.DataC[0] = 'R';
		sendMessage(query);
		gettimeofday(&lastQRCurTime, NULL);
		return;
	}
	elsaped = getMilliSecsBetween(lastQLCurTime, curtime);
	if(elsaped > CURRENT_PERIOD) {
		Message query;
		query.type = CURRENT_TYPE;
		query.DataC[0] = 'L';
		sendMessage(query);
		gettimeofday(&lastQLCurTime, NULL);
		return;
	}
	elsaped = getMilliSecsBetween(lastQVoltTime, curtime);
	if(elsaped > QUERY_PERIOD) {
		Message query;
		query.type = VOLTAGE_TYPE;
		sendMessage(query);
		gettimeofday(&lastQVoltTime, NULL);
		return;
	}
}

void MotorControllerHandler::CheckMotor() {
	if(awaitingResponse)
		return;
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int elsaped = getMilliSecsBetween(lastMotorTime, curTime);
	if(elsaped < MOTOR_PERIOD)
		return;

	bool needsSent = true;
	int leftSetSpeed = leftSpeed;
	int rightSetSpeed = rightSpeed;
	if(leftSpeed != leftTargetSpeed) {
		int diff = leftTargetSpeed - leftSpeed;
		if(diff > MaxStep)
			diff = MaxStep;
		if(diff < -MaxStep)
			diff = -MaxStep;
		leftSetSpeed += diff;
		needsSent=true;
	}
	if(rightSpeed != rightTargetSpeed) {
		int diff = rightTargetSpeed - rightSpeed;
		if(diff > MaxStep)
			diff = MaxStep;
		if(diff < -MaxStep)
			diff = -MaxStep;
		rightSetSpeed += diff;
		needsSent=true;
	}
	if(needsSent) {
		sendMessage(createMessageFromSpeed(rightSetSpeed, leftSetSpeed));
		gettimeofday(&lastMotorTime, NULL);
	}
}

void MotorControllerHandler::spinOnce() {
	receive();
	CheckMotor();
	CheckQuery();
	if(awaitingResponse && TransmitTimeout()) {
		transmit();
	}
}
