#pragma once

#include <ihmc_msgs/PelvisHeightTrajectoryRosMessage.h>
#include <ros/ros.h>

class pelvisTrajectory {

private:

    ros::NodeHandle nh_;
    ros::Publisher pelvisHeightPublisher;
    static int pelvis_id;

public:

    pelvisTrajectory(ros::NodeHandle nh);
    ~pelvisTrajectory();
    void controlPelvisHeight(float height);
    bool controlPelvisMessage(ihmc_msgs::PelvisHeightTrajectoryRosMessage msg);
};
    int pelvisTrajectory::pelvis_id = -1;
