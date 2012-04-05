#!/bin/bash

##Stage 1
#roscore &

#sleep 2

##Stage 2

# cameras driver
roslaunch SubCameraDriver cameras.launch &

# start the sensor board
rosrun SubSensorController SubSensorController /dev/controller_sensor &

# start the imu
rosrun SubImuController SubImuController /dev/controller_Imu &

# start the motor controllers
rosrun SubMotorController SubMotorController &

# start the moboTemp module
rosrun moboTemp moboTemp &

#Translation from ticks to actual depth
rosrun SubTranslators DepthTranslator

sleep 1

##Stage 3

# republish cameras as compressed for recording
/opt/ros/diamondback/stacks/image_common/image_transport/bin/republish raw in:=left/image_raw compressed out:=left/image_compressed &
/opt/ros/diamondback/stacks/image_common/image_transport/bin/republish raw in:=right/image_raw compressed out:=right/image_compressed &

# image recognition
/opt/robosub/rosWorkspace/SubImageRecognition/src/image_recognition.py &

sleep 1

##Stage 4

# republish image recognition as compressed for viewing remotely
/opt/ros/diamondback/stacks/image_common/image_transport/bin/republish raw in:=forward_camera/image_raw compressed out:=forward_camera/image_compressed &
/opt/ros/diamondback/stacks/image_common/image_transport/bin/republish raw in:=downward_camera/image_raw compressed out:=downward_camera/image_compressed &

# save compressed cameras in a bag
rosbag record -O /home/robosub/bags/cameras.`date +%Y%m%d%H%M`.bag left/image_compressed left/image_compressed/compressed right/image_compressed right/image_compressed/compressed &

#Simple Depth Controller maintains a target depth
rosrun subSim simpleDepth

