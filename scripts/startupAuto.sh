#!/bin/bash

set -eu

LOGFILE=/home/robosub/roslog.log
PIDFILE=/home/robosub/rossession.pid

exec 3>&1
echo "Writing to $LOGFILE"

exec 1>$LOGFILE
#exec 2>&1 

echo "---------------------------"
echo "SESSION: $( date )"

## Stage 1


roscore &

sleep 2

## Stage 2

# Start camera drivers
echo "Start camera drivers" >&3
#roslaunch pgr_camera_driver camera_node.launch camera_node:=left camera_name:='stereo/left' serialnumber:=13021177 calibration_file:='file:///opt/robosub/rosWorkspace/pgr_camera_driver/stereo_intrinsics.ini' &
roslaunch pgr_camera_driver camera_node.launch camera_node:=right camera_name:='stereo/right' serialnumber:=12460898 calibration_file:='file:///opt/robosub/rosWorkspace/pgr_camera_driver/stereo_intrinsics.ini' &


##Run stereo image processing

##ROS_NAMESPACE=stereo rosrun stereo_image_proc stereo_image_proc &

#We want to launch the driver first.
##roslaunch SubCameraDriver camera.launch &  

# Start the sensor board
echo "Start the sensor board" >&3
rosrun SubSensorController SubSensorController /dev/controller_sensor &  

# Start the imu
echo "Start the IMU" >&3
#rosrun SubAttitudeResolver SubAttitudeResolver /dev/controller_Imu &
rosrun SubImuController SubImuController /dev/controller_Imu > /home/robosub/SubImuController.log  &  

# Start the motor controllers
echo "Start the motor controller"
roslaunch SubMotorController MotorController.launch &  


# Start the high level contorl
rosrun SubMotorController SubHighLevelMotorController &  

# Start the moboTemp module
#rosrun moboTemp moboTemp &  

echo "Start depth translator" >&3
# Translation from ticks to actual depth
rosrun SubTranslators DepthTranslator &  

#echo "Start state machine" >&3
rosrun PathTask PathTask.py &

rosrun Brain Brain.py &

echo "Start depth controller"
# Simple Depth Controller maintains a target depth
#rosrun subSim simpleDepth &  
rosrun SubDepthController SubDepthController &
# Simple Heading Controller maintains a target heading
#rosrun subSim simpleHeading &  
rosrun SubHeading SubHeading &
sleep 4

## Stage 3

echo "Republish camera topics" >&3
# Republish cameras as compressed for recording

#/opt/ros/fuerte/stacks/image_common/image_transport/bin/republish raw in:=/stereo/left/image_raw compressed out:=/stereo/left/image_compressed &  

/opt/ros/fuerte/stacks/image_common/image_transport/bin/republish raw in:=/stereo/right/image_raw compressed out:=/stereo/right/image_compressed &  

#/opt/ros/fuerte/stacks/image_common/image_transport/bin/republish raw in:=image_raw compressed out:=image_compressed &  

sleep 5
echo "Start image recognition" >&3
# Image recognition
/opt/robosub/rosWorkspace/SubImageRecognition/bin/ImageRecognition &  

# Dive Alarm
#rosrun SubDiveAlarm SubDiveAlarm &

sleep 2

## Stage 4

# Republish image recognition as compressed for viewing remotely

/opt/ros/fuerte/stacks/image_common/image_transport/bin/republish raw in:=forward_camera/image_raw compressed out:=forward_camera/image_compressed &  

/opt/ros/fuerte/stacks/image_common/image_transport/bin/republish raw in:=downward_camera/image_raw compressed out:=downward_camera/image_compressed &  


# Save compressed cameras and resulting recognition info in a bag
#rosbag record -O /home/robosub/bags/cameras.`date +%Y%m%d%H%M`.bag left/image_compressed left/image_compressed/compressed right/image_compressed right/image_compressed/compressed image_recognition/forward/buoys image_recognition/forward/gate image_recognition/v
#echo Starting camera bag
rosbag record -O /home/robosub/bags/cameras.`date +%Y%m%d%H%M`.bag /stereo/right/image_compressed/compressed /stereo/left/image_compressed/compressed downward_camera/image_compressed/compressed image_recognition/forward/buoys image_recognition/forward/gate image_recognition/downward/orange_rectangles &  

#echo Starting sensor bag
# Save sensor data in a bag
rosbag record -O /home/robosub/bags/sensors.`date +%Y%m%d%H%M`.bag Calibrate_Depth Computer_Cur_Volt Controller_Box_Temp Error_Log IMU_Attitude IMU_Accel_Debug IMU_Gyro_Debug Mobo_Temp Motor_Control Motor_State Motor_Current Pressure_Data Sub_Depth Target_Depth Water_Detected Points_Of_Interest High_Level_Control Target_Heading & 

echo "Calibrating depth to zero">&3
# Calibrate the current pressure as 0
rostopic pub /Calibrate_Depth std_msgs/Float32 -1 -- 0.0 &
disown $!

guvcview
sleep 2

roslaunch SubCameraDriver camera.launch &

sleep 2

exec 1>&3
jobs -p > $PIDFILE

echo "Runnning processes:"
jobs -l

## Complete
echo "\nStartup Complete!"
echo "To tail the log file run 'roslog'"
echo "To kill all processes run 'killRos'"
