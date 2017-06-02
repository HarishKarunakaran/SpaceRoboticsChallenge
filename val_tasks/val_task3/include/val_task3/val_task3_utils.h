#ifndef VAL_TASK3_UTILS_H
#define VAL_TASK3_UTILS_H

#include <ros/ros.h>
#include <srcsim/Satellite.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <val_common/val_common_names.h>
#include <val_controllers/val_chest_navigation.h>
#include <val_controllers/val_pelvis_navigation.h>
#include <val_controllers/val_gripper_control.h>
#include <val_controllers/val_head_navigation.h>
#include <val_controllers/val_arm_navigation.h>


class task3Utils{
private:
    ros::NodeHandle nh_;
    armTrajectory arm_controller_;

    const std::vector<float> RIGHT_ARM_SEED_TABLE_MANIP = {-0.23, 0, 0.70, 1.51, -0.05, 0, 0};
    const std::vector<float> LEFT_ARM_SEED_TABLE_MANIP  = {-0.23, 0, 0.70, -1.51, 0.05, 0, 0};
    const std::vector<float> RIGHT_ARM_DOOR_OPEN = {-1.35, 1.20, 0.75, 0.50, 1.28, 0.0, 0.0};
    const std::vector<float> LEFT_ARM_DOOR_OPEN  = {-0.40, -1.25, 0.70,-0.68, 1.27, 0, 0};

public:
    task3Utils(ros::NodeHandle nh);
    ~task3Utils();
    void beforePanelManipPose();
    void beforDoorOpenPose();

};






















#endif // VAL_TASK3_UTILS_H
