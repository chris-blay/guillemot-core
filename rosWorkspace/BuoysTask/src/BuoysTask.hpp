#ifndef _BUOYS_TASK_HPP
#define _BUOYS_TASK_HPP

#include <list>
#include "ros/ros.h"
#include "std_msgs/UInt8.h"
#include "Robosub/ModuleEnableMsg.h"
#include "Robosub/HighLevelControl.h"
#include "SubImageRecognition/ImgRecObject.h"

enum BuoyColors
{
    GREEN = 0,
    YELLOW = 1,
    RED = 2
};

struct BuoyData
{
    ros::Time sampleTime;
    short centerX;
    short centerY;
    short height;
    short width;
    short confidence;
};

class BuoysTask
{
public:
    BuoysTask(BuoyColors first, BuoyColors second);
    ~BuoysTask();

    void moduleEnableCallback(const Robosub::ModuleEnableMsg& msg);
    void greenBuoyCallback(const SubImageRecognition::ImgRecObject& msg);
    void yellowBuoyCallback(const SubImageRecognition::ImgRecObject& msg);
    void redBuoyCallback(const SubImageRecognition::ImgRecObject& msg);
    bool centerOnBuoy(BuoyColors color);
    void bumpBuoy(BuoyColors color);
    float getDistanceToBuoy(BuoyColors buoy);
    void clearPastSamples(void);
    bool isBuoyVisible(BuoyColors buoy);
    void searchBuoys(BuoyColors buoy);

private:
    bool performTask(void);

    std::list<BuoyData> m_greenBuoySamples;
    std::list<BuoyData> m_yellowBuoySamples;
    std::list<BuoyData> m_redBuoySamples;
    ros::NodeHandle m_nodeHandle;
    ros::Subscriber m_greenBuoySubcriber;
    ros::Subscriber m_yellowBuoySubcriber;
    ros::Subscriber m_redBuoySubcriber;
    ros::Subscriber m_taskStateSubscriber;
    ros::Publisher m_highLevelMotorPublisher;
    ros::Publisher m_centerOnPointPublisher;
    ros::Publisher m_taskCompletePublisher;
    BuoyColors m_firstToBump;
    BuoyColors m_secondToBump;
    std::list<BuoyData>* m_pFirstBumpList;
    std::list<BuoyData>* m_pSecondBumpList;
    bool m_isEnabled;

};

#endif // _BUOYS_TASK_HPP
