#include <QPoint>
#include<QDesktopWidget>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QPainter>
#include <QDebug>
#include <iostream>
#include <math.h>
#include "qwt/qwt_dial_needle.h"

#include "SubConsole.hpp"
#include "USUbConsole/MotorMessage.h"
#include "SubImageRecognition/UpdateAlgorithm.h"
#include "SubImageRecognition/ListAlgorithms.h"
#include "SubImageRecognition/ImgRecThreshold.h"
#include "ui_SubConsole.h"

/**
 * @brief SubConsole ctor which sets up timers and connect signals/slots
 *
 * @param pParent Poitner to parent widget
 **/
SubConsole::SubConsole(QWidget* pParent)
   : QMainWindow(pParent),
     m_pUi(new Ui::SubConsole),
     m_pJoystickTimer(new QTimer(this)),
     m_pCallbackTimer(new QTimer(this)),
     m_pJoystick(new Joystick()),
     m_nodeHandle(),
     m_motorDriverPublisher(),
     m_depthPublisher(),
     m_thresholdBoxPublisher(),
     m_imageRecService(),
     m_listAlgorithmService(),
     m_imuSubscriber(),
     m_motorControllerTempSubscriber(),
     m_moboTempSubscriber(),
     m_pressureSubscriber(),
     m_depthSubscriber(),
     m_forwardCameraSubscriber(),
     m_downwardCameraSubscriber(),
     m_voltageCurrentSubscriber(),
     m_errorLogSubscriber(),
     m_lastXAxisValue(0),
     m_lastYAxisValue(0),
     m_lastThrottleValue(0),
     m_lastTwistValue(0),
     m_turnForwardPercentage(1.0),
     m_rightThrustPercentage(1.0),
     m_leftThrustPercentage(1.0),
     m_pForwardCameraData(NULL),
     m_pDownwardCameraData(NULL),
     m_downPipEnabled(false),
     m_forwardPipEnabled(false),
     m_pCompass(NULL),
     m_pPitchIndicator(NULL),
     m_pRollIndicator(NULL),
     m_pImageRecBoxLabel(NULL)
{
   m_pUi->setupUi(this);
   m_pJoystickTimer->setInterval(JOYSTICK_POLL_INTERVAL_MSEC);
   m_pCallbackTimer->setInterval(CALLBACK_HANDLE_INTERVAL_MSEC);

   connect(m_pJoystickTimer, SIGNAL(timeout()), this, SLOT(readJoystickInput()));
   connect(m_pCallbackTimer, SIGNAL(timeout()), this, SLOT(handleRosCallbacks()));
   connect(m_pUi->connectButton, SIGNAL(clicked()), this, SLOT(joyConnect()));
   connect(m_pUi->downPipButton, SIGNAL(clicked()), this, SLOT(toggleDownwardPiP()));
   connect(m_pUi->forwardPipButton, SIGNAL(clicked()), this, SLOT(toggleForwardPiP()));
   connect(m_pUi->toggleBoxThresholdButton, SIGNAL(clicked()), this, SLOT(toggleBoxThresholding()));
   connect(m_pUi->enableViewThresholdButton, SIGNAL(clicked()), this, SLOT(enableViewThresholds()));
   connect(m_pUi->disableViewThresholdButton, SIGNAL(clicked()), this, SLOT(disableViewThresholds()));

   m_pCallbackTimer->start();

   //Attempt to connect to default joystick location
   joyConnect();

   //Center window on screen
   move(qApp->desktop()->availableGeometry(this).center()-rect().center());

   m_pUi->forwardCameraImageThumb->hide();
   m_pUi->downCameraImageThumb->hide();

   m_motorDriverPublisher = m_nodeHandle.advertise<USUbConsole::MotorMessage>("Motor_Control", 100);
   m_depthPublisher = m_nodeHandle.advertise<std_msgs::Float32>("Target_Depth", 100);
   m_thresholdBoxPublisher = m_nodeHandle.advertise<SubImageRecognition::ImgRecThreshold>("Threshold_Box", 100);
   m_imageRecService = m_nodeHandle.serviceClient<SubImageRecognition::UpdateAlgorithm>("img_rec/update_algorithm");
   m_listAlgorithmService = m_nodeHandle.serviceClient<SubImageRecognition::ListAlgorithms>("img_rec/list_algorithms");

   m_imuSubscriber = m_nodeHandle.subscribe("IMU_Attitude", 100, &SubConsole::imuDataCallback, this);
   m_motorControllerTempSubscriber = m_nodeHandle.subscribe("Controller_Box_Temp", 100, &SubConsole::motorControllerTempCallback, this);
   m_moboTempSubscriber = m_nodeHandle.subscribe("Mobo_Temp", 100, &SubConsole::moboTempCallback, this);
   m_pressureSubscriber = m_nodeHandle.subscribe("Pressure_Data", 100, &SubConsole::pressureDataCallback, this);
   m_depthSubscriber = m_nodeHandle.subscribe("Sub_Depth", 100, &SubConsole::depthCallback, this);
   m_forwardCameraSubscriber = m_nodeHandle.subscribe("/forward_camera/image_compressed/compressed", 100, &SubConsole::forwardCameraCallback, this);
   m_downwardCameraSubscriber = m_nodeHandle.subscribe("/downward_camera/image_compressed/compressed", 100, &SubConsole::downwardCameraCallback, this);
   m_voltageCurrentSubscriber = m_nodeHandle.subscribe("Computer_Cur_Volt", 100, &SubConsole::currentVoltageCallback, this);
   m_errorLogSubscriber = m_nodeHandle.subscribe("Error_Log", 100, &SubConsole::errorLogCallback, this);

   printf("Finished ROS topic publish and subscription initialization\n");

   m_pCompass = new QwtCompass(m_pUi->imuGroupBox);
   m_pCompass->resize(80, 80);
   m_pCompass->move(40, 25);
   m_pCompass->setNeedle(new QwtCompassMagnetNeedle());

   m_pPitchIndicator = new AttitudeIndicator(m_pUi->imuGroupBox);
   m_pPitchIndicator->resize(80, 80);
   m_pPitchIndicator->move(165, 25);

   m_pRollIndicator = new AttitudeIndicator(m_pUi->imuGroupBox);
   m_pRollIndicator->resize(80, 80);
   m_pRollIndicator->move(290, 25);

   m_pImageRecBoxLabel = new ClickableLabel(m_pUi->forwardCameraImage);
   m_pImageRecBoxLabel->resize(478, 638);
   m_pImageRecBoxLabel->move(1, 1);
   connect(m_pImageRecBoxLabel, SIGNAL(clicked()), this, SLOT(imageRecThresholdBoxDrawn()));
}

/**
 * @brief SubConsole dtor
 **/
SubConsole::~SubConsole()
{
   delete m_pCompass;
   delete m_pPitchIndicator;
   delete m_pRollIndicator;
   delete m_pUi;

   m_pJoystickTimer->stop();
   m_pCallbackTimer->stop();
   delete m_pJoystickTimer;
   delete m_pCallbackTimer;
   delete m_pJoystick;
}

/**
 * @brief Attempts to connect to the joystick
 **/
void SubConsole::joyConnect(void)
{
   m_pJoystickTimer->stop();

   if(m_pJoystick->init(m_pUi->devLineEdit->text().toStdString().c_str()) == 0)
   {
       m_pUi->joystickStatusLabel->setText("Connected");
       m_pJoystickTimer->start();
   }
   else
   {
       m_pUi->joystickStatusLabel->setText("Disconnected");
   }
}

/**
 * @brief Periodically reads joystick states if any states have changed publish topic to motor controllers
 **/
void SubConsole::readJoystickInput(void)
{
   int currentXAxis = m_pJoystick->getAxis(0);
   int currentYAxis = m_pJoystick->getAxis(1);
   int currentTwistAxis = m_pJoystick->getAxis(2);
   int currentThrottleAxis = m_pJoystick->getAxis(3);

   unsigned char motorMask = 0;
   short leftDriveValue = 0;
   short rightDriveValue = 0;
   short frontTurnValue = 0;
   short rearTurnValue = 0;
   short frontDepthValue = 0;
   short rearDepthValue = 0;

   if(m_lastXAxisValue != currentXAxis)   //Strafe
   {
       //Set the horizontal thrusters to opposite thrust to strafe
       int turnSpeed = 255 * (abs(currentXAxis) / (double)JOYSTICK_MAX_VALUE);

       if(currentXAxis > 0)  //Strafe right
       {
          frontTurnValue = turnSpeed * -1;
          rearTurnValue = turnSpeed * 0.93;
       }
       else if(currentXAxis < 0)//Strafe left
       {
          frontTurnValue = turnSpeed * 0.85;
          rearTurnValue = turnSpeed * -1;
       }

       motorMask |= (MOTOR_FRONT_TURN | MOTOR_REAR_TURN);

       m_lastXAxisValue = currentXAxis;
   }

   if(m_lastYAxisValue != currentYAxis)   //Move forward/backwards
   {
      //Set the forward/reverse thrusters to same value to move forwards or backwards
      int thrusterSpeed = 255 * (abs(currentYAxis) / (double)JOYSTICK_MAX_VALUE);

      //A neg number means the stick is pushed forward, if positive we actually want reverse
      if(currentYAxis < 0)
      {
         leftDriveValue = thrusterSpeed * m_leftThrustPercentage;
         rightDriveValue = thrusterSpeed * m_rightThrustPercentage;
      }
      else if(currentYAxis > 0)
      {
          leftDriveValue = thrusterSpeed  * -1 * m_leftThrustPercentage;
          rightDriveValue = thrusterSpeed  * -1 * m_rightThrustPercentage;
      }

      motorMask |= (MOTOR_LEFT_DRIVE | MOTOR_RIGHT_DRIVE);

      m_lastYAxisValue = currentYAxis;
   }

   if(m_lastTwistValue != currentTwistAxis)  //Turn
   {
      //Set the horizontal thrusters to the same direction/velocity to rotate sub
      int thrusterSpeed = 255 * (abs(currentTwistAxis) / (double)JOYSTICK_MAX_VALUE);

      if(currentTwistAxis > 0)  //Turn right, set both thrusters to reverse
      {
          frontTurnValue = thrusterSpeed * 0.85;
          rearTurnValue = thrusterSpeed * 0.93;

      }
      else if(currentTwistAxis < 0)    //Turn left, set both thrusters to forward
      {
          frontTurnValue = thrusterSpeed * -1;
          rearTurnValue = thrusterSpeed * -1;
      }

      motorMask |= (MOTOR_FRONT_TURN | MOTOR_REAR_TURN);

      m_lastTwistValue = currentTwistAxis;
   }

   if(m_lastThrottleValue != currentThrottleAxis)  //Submerge/surface
   {
      if(m_pUi->depthCheckBox->isChecked())
      {
          m_lastThrottleValue = currentThrottleAxis;
          currentThrottleAxis += JOYSTICK_MAX_VALUE;

          float desiredDepth = MAXIMUM_DEPTH * (currentThrottleAxis / (double)(JOYSTICK_MAX_VALUE * 2));
          std_msgs::Float32 depthMsg;

          depthMsg.data = desiredDepth;

          m_pUi->targetDepthLineEdit->setText(QString::number(desiredDepth));

          m_depthPublisher.publish(depthMsg);
      }
      else
      {
          //Set the vertical thrusters to the same value to control depth
          int thrusterSpeed = 255 * (abs(currentThrottleAxis) / (double)JOYSTICK_MAX_VALUE);

          if(currentThrottleAxis < 0)
          {
             frontDepthValue = thrusterSpeed;
             rearDepthValue = thrusterSpeed;
          }
          else if(currentThrottleAxis > 0)
          {
              frontDepthValue = thrusterSpeed * -1;
              rearDepthValue = thrusterSpeed * -1;
          }

          motorMask |= (MOTOR_FRONT_DEPTH | MOTOR_REAR_DEPTH);

          m_lastThrottleValue = currentThrottleAxis;
      }
   }

   if(motorMask != 0x0)
   {
     sendMotorSpeedMsg(motorMask, leftDriveValue, rightDriveValue, frontDepthValue, rearDepthValue, frontTurnValue, rearTurnValue);
   }
}

/**
 * @brief Periodically allows ROS time to handle received callbacks
 **/
void SubConsole::handleRosCallbacks(void)
{
    ros::spinOnce();
}

/**
 * @brief Publishes a Motor_Driver message
 *
 * @param motorMask Bit mask with the motors to drive at the specified speed
 **/
void SubConsole::sendMotorSpeedMsg(unsigned char motorMask, short leftDrive, short rightDrive, short frontDepth, short rearDepth, short frontTurn, short rearTurn)
{
   USUbConsole::MotorMessage motorMsg;

   motorMsg.mask = motorMask;
   motorMsg.Left = leftDrive;
   motorMsg.Right = rightDrive;
   motorMsg.FrontDepth = frontDepth;
   motorMsg.RearDepth = rearDepth;
   motorMsg.FrontTurn = frontTurn;
   motorMsg.RearTurn = rearTurn;

   m_motorDriverPublisher.publish(motorMsg);
}

/**
 * @brief ROS callback for IMU_Attitude subscription
 *
 * @param msg The received message
 **/
void SubConsole::imuDataCallback(const std_msgs::Float64MultiArray::ConstPtr& msg)
{
   m_pUi->yawLineEdit->setText(QString::number(msg->data[0]));
   m_pCompass->setValue(msg->data[0]);

   m_pUi->pitchLineEdit->setText(QString::number(msg->data[1]));
   m_pPitchIndicator->setGradient(msg->data[1]/90.0);

   m_pUi->rollLineEdit->setText(QString::number(msg->data[2]));
   m_pRollIndicator->setAngle(msg->data[2]);
}

/**
 * @brief ROS callback for Controller_Box_Temp subscription
 *
 * @param msg The received message
 **/
void SubConsole::motorControllerTempCallback(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
   m_pUi->motorController1TempLineEdit->setText(QString::number((int)msg->data[0]));
   m_pUi->motorController2TempLineEdit->setText(QString::number((int)msg->data[1]));
   m_pUi->motorController3TempLineEdit->setText(QString::number((int)msg->data[2]));
}

/**
 * @brief ROS callback for Mobo_Temp subscription
 *
 * @param msg The received message
 **/
void SubConsole::moboTempCallback(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
   m_pUi->moboTempLineEdit->setText(QString::number(msg->data[0]));
   m_pUi->moboTemp2LineEdit->setText(QString::number(msg->data[1]));
}

/**
 * @brief ROS callback for Pressure_Data subscription
 *
 * @param msg The received message
 **/
void SubConsole::pressureDataCallback(const std_msgs::Float32::ConstPtr& msg)
{
   m_pUi->ticksDepthLineEdit->setText(QString::number(msg->data));
}

/**
 * @brief ROS callback for Sub_Depth subscription
 *
 * @param msg The received message
 **/
void SubConsole::depthCallback(const std_msgs::Float32::ConstPtr& msg)
{
   m_pUi->depthLineEdit->setText(QString::number(msg->data));
}

/**
 * @brief ROS callback for Computer_Cur_Volt subscription
 *
 * @param msg The received message
 **/
void SubConsole::currentVoltageCallback(const std_msgs::Float32MultiArray::ConstPtr& msg)
{
   m_pUi->currentLineEdit->setText(QString::number(msg->data[0]));
   m_pUi->voltageLineEdit->setText(QString::number(msg->data[1]));
}

 void SubConsole::errorLogCallback(const std_msgs::String::ConstPtr& msg)
 {
     m_pUi->errorLogTextEdit->appendPlainText(QString::fromStdString(msg->data));
 }

/**
 * @brief ROS callback for Forward_Camera subscription
 *
 * @param msg The received message
 **/
void SubConsole::forwardCameraCallback(const sensor_msgs::CompressedImage::ConstPtr& msg)
{
   QImage image;
   QPixmap pixmap;

   if(m_pForwardCameraData != NULL)
   {
       delete [] m_pForwardCameraData;
   }

   m_pForwardCameraData = new unsigned char[msg->data.size()];
   std::copy(msg->data.begin(), msg->data.end(), m_pForwardCameraData);

   image.loadFromData(m_pForwardCameraData, msg->data.size(), "JPG");
   m_pUi->forwardCameraImage->setPixmap(pixmap.fromImage(image, 0));

   if(m_forwardPipEnabled)
   {
      m_pUi->forwardCameraImageThumb->setPixmap(pixmap.fromImage(image.scaledToHeight(192), 0));
   }
}

/**
 * @brief ROS callback for Forward_Camera subscription
 *
 * @param msg The received message
 **/
void SubConsole::downwardCameraCallback(const sensor_msgs::CompressedImage::ConstPtr& msg)
{
    QImage image;
    QPixmap pixmap;

    if(m_pDownwardCameraData != NULL)
    {
        delete [] m_pDownwardCameraData;
    }

    m_pDownwardCameraData = new unsigned char[msg->data.size()];
    std::copy(msg->data.begin(), msg->data.end(), m_pDownwardCameraData);

    image.loadFromData(m_pDownwardCameraData, msg->data.size(), "JPG");
    m_pUi->downwardCameraImage->setPixmap(pixmap.fromImage(image, 0));

    if(m_downPipEnabled)
    {
        m_pUi->downCameraImageThumb->setPixmap(pixmap.fromImage(image.scaledToHeight(192), 0));
    }
}

/**
 * @brief Toggles the visibility of the downward picture-in-picture
 **/
void SubConsole::toggleDownwardPiP(void)
{
    if(m_downPipEnabled)
    {
        m_pUi->downCameraImageThumb->hide();
        m_downPipEnabled = false;
    }
    else
    {
        m_pUi->downCameraImageThumb->show();
        m_downPipEnabled = true;
    }
}

/**
 * @brief Toggles the visibility of the forward camera picture-in-picture
 **/
void SubConsole::toggleForwardPiP(void)
{
    if(m_forwardPipEnabled)
    {
        m_pUi->forwardCameraImageThumb->hide();
        m_forwardPipEnabled = false;
    }
    else
    {
        m_pUi->forwardCameraImageThumb->show();
        m_forwardPipEnabled = true;
    }
}

void SubConsole::toggleBoxThresholding(void)
{
    if (m_pUi->toggleBoxThresholdButton->text() == "Enable Box Thresholding")
    {
        m_pUi->toggleBoxThresholdButton->setText("Disable Box Thresholding");
        m_pImageRecBoxLabel->rectangleDrawState(true);
    }
    else
    {
        m_pUi->toggleBoxThresholdButton->setText("Enable Box Thresholding");
        m_pImageRecBoxLabel->rectangleDrawState(false);
        m_pImageRecBoxLabel->clearRectangle();
    }
}

void SubConsole::enableViewThresholds(void)
{
    SubImageRecognition::UpdateAlgorithm::Request updateAlgorithmService;
    SubImageRecognition::UpdateAlgorithm::Response updateAlgorithmResponse;

    updateAlgorithmService.algorithm.name = getSelectedAlgorithm();
    updateAlgorithmService.algorithm.flags = 2;

    if (m_imageRecService.call(updateAlgorithmService, updateAlgorithmResponse))
    {
        // printf("Update and view thresholds status: %i\n", updateAlgorithmService.response.result);
    }
    else
    {
        printf("Error, failed to send update algorithm service\n");
    }
}

void SubConsole::disableViewThresholds(void)
{
    SubImageRecognition::UpdateAlgorithm::Request updateAlgorithmService;
    SubImageRecognition::UpdateAlgorithm::Response updateAlgorithmResponse;

    updateAlgorithmService.algorithm.name = getSelectedAlgorithm();
    updateAlgorithmService.algorithm.flags = 0; // @todo What do set flags to to disable threshold view without disabling algorithm

    if (m_imageRecService.call(updateAlgorithmService, updateAlgorithmResponse))
    {
        // printf("Update and view thresholds status: %i\n", updateAlgorithmService.response.result);
    }
    else
    {
        printf("Error, failed to send update algorithm service\n");
    }
}

std::string SubConsole::getSelectedAlgorithm(void)
{
    std::string selected = "";

    if (m_pUi->algorithmComboBox->currentText() == "Red Buoy")
    {
        selected = "buoys/red";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Green Buoy")
    {
        selected = "buoys/green";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Yellow Buoy")
    {
        selected = "buoys/yellow";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Gate")
    {
        selected = "gate";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Obstable Course")
    {
        selected = "obstacle_course";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Torpedo Target")
    {
        selected = "torpedo";
    }
    else if (m_pUi->algorithmComboBox->currentText() == "Path")
    {
        selected = "paths";
    }

    return selected;
}

void SubConsole::imageRecThresholdBoxDrawn(void)
{
    SubImageRecognition::ImgRecThreshold thresholdMsg;

    thresholdMsg.name = getSelectedAlgorithm();
    thresholdMsg.x1 = m_pImageRecBoxLabel->getX1();
    thresholdMsg.y1 = m_pImageRecBoxLabel->getY1();
    thresholdMsg.x2 = m_pImageRecBoxLabel->getX2();
    thresholdMsg.y2 = m_pImageRecBoxLabel->getY2();

    if(thresholdMsg.name != "")
    {
        m_thresholdBoxPublisher.publish(thresholdMsg);
    }
}
