#include "ros/ros.h"
#include "std_msgs/Float32.h"
#include "motor.h"
#include <Robosub/ModuleEnableMsg.h>
#include <sys/time.h>

#define OFF 0
#define ON 1

float targetDepth = 0;

const int LEFT_MOTOR = 0x400;
const int RIGHT_MOTOR = 0x200;
const int REVERSED = 0x100;

const double KI = 0.5;
const double KP = 1.0;
//Initial contidions
float yOld = 0;
float eOld = 0;
struct timeval tOld;
bool MODE = ON;

double pitch = 0.0;



ros::Publisher depthMotor;

void mTargetDepthCallback(const std_msgs::Float32::ConstPtr& msg) {
	targetDepth = msg->data;
	printf("setting target depth to %f\n", targetDepth);
}

void setDepthSpeed(float speed) {
	speed *= 255;
	int speedInt = speed; //truncate to an integer value
	if(speedInt > 255)
		speedInt = 255;
	if(speedInt < -255)
		speedInt = -255;
//	setMotors(REAR_DEPTH_MOTOR_BIT | FRONT_DEPTH_MOTOR_BIT,  //<- bit mask
//			0,        0,          //<- Drive motors (ignored because of mask)
//			speedInt, speedInt,   //<- Depth motors
//			0,        0           //<- Turn motors  (also ignored)
//		);
	setMotors(REAR_DEPTH_MOTOR_BIT | FRONT_DEPTH_MOTOR_BIT,
			0,        0,
			-speedInt, -speedInt,
			0,        0
		);
}

void mEnabledCallback(const Robosub::ModuleEnableMsg::ConstPtr& msg) {
	if(msg->Module == "Simple_Depth")
		MODE = msg->State;
}
//TODO Add the stabilizer: A PI controller?
//TODO Make the stabilizer independent and add the output to the motor signal
//TODO Make the stabilizer output differential (so it can add or substract from the motor signal)

void mAttitudeCallback(const std_msgs::Float32MultiArray::ConstPtr& msg){
    if(MODE == OFF)
        return;
   //double yaw = msg->data[2];
    pitch = msg->data[1];
   //double roll = msg->data[0];

}

void mCurrentDepthCallback(const std_msgs::Float32::ConstPtr& msg) {
	if(MODE == OFF)
		return;
	float depth = msg->data;
	float error = 0;
	//Error calculation
//	if(depth - targetDepth > 1.5)
//		error = 1;
//	else if(depth - targetDepth < -1.5)
//		error = -1;
//	else
	error = depth - targetDepth;
    if (error<0.1){ //Could change
        error = 0;
        yOld = 0; //Should help avoid drift caused for small differences
        eOld = 0;
    }

    //Proportional
    p = KP*error;
    //Integral
    if (yOld==0 && eOld==0){
        //reset timer
        gettimeofday(&tOld,0);
    }

    struct timeval tNow;
    gettimeofday(&tNow,0);

    t = tNow-tOld;
    float y = yOld + KI*t/2*(error+eOld); //should be <1

    //Update buffers
    yOld = y;
    eOld = error;
    tOld.tv_sec = tNow.tv_sec;
    tOld.tv_usec = tNow.tv_usec;

    speed = p+y;
    if (speed > 1)
        speed = 1;
    else if(speed < -1)
        speed = -1;

	setDepthSpeed(speed);
}

int main(int argc, char** argv) {
	ros::init(argc, argv, "SimpleDepthController");
	ros::NodeHandle nh;
	ros::Subscriber curDepth = nh.subscribe("Sub_Depth", 1, mCurrentDepthCallback);
	ros::Subscriber targetDepth = nh.subscribe("/Target_Depth", 1, mTargetDepthCallback);
	ros::Subscriber imuAttitude = nh.subscribe("/IMU_Attitude", 1, mAttitudeCallback);
	ros::Subscriber enabled = nh.subscribe("/Module_Control", 1, mEnabledCallback);
	ros::spin();
}
