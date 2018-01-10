#ifndef BUTTON_PRESS_H
#define BUTTON_PRESS_H
#include <src_task2/button_detector.h>
#include <tough_controller_interface/arm_control_interface.h>
#include <tough_controller_interface/gripper_control_interface.h>
#include <tough_footstep/RobotWalker.h>
#include <tough_common/val_common_defines.h>
#include <tough_common/val_common_names.h>
#include "tough_moveit_planners/tough_cartesian_planner.h"
#include "tough_controller_interface/wholebody_control_interface.h"
#include "tough_controller_interface/chest_control_interface.h"


class ButtonPress
{
    ArmControlInterface armTraj_;
    ros::NodeHandle nh_;
//    ButtonDetector bd_;
    GripperControlInterface gripper_;
    RobotStateInformer *current_state_;
    RobotDescription *rd_;
    geometry_msgs::QuaternionStamped leftHandOrientation_ ;
    geometry_msgs::QuaternionStamped rightHandOrientation_;

    const std::vector<float> leftShoulderSeed_ = {-1.06 ,-0.77, 0.70, -1.12 ,1.96, 0.0, 0.0};
    const std::vector<float> rightShoulderSeed_ = {-0.23, 0.72, 0.65 , 1.51, 2.77, 0.0, 0.0};

    /*New Approach*/
    const std::vector<float> leftShoulderSeedInitial_ = {-0.86,0.20,1.02,-1.67,1.40,0.0,0.0};

//    const std::vector<float> leftShoulderSeedInitial_ = {-1.59, -0.55, 1.18, -1.05, 1.52, -0.01, 0.0};
    const std::vector<float> rightShoulderSeedInitial_ = {-0.81,0.19,0.65,1.49,1.29,0.0,0.0};

    //    // Not in use currently
    //    const std::vector<float> leftShoulderSeed_ = {-0.23 ,-1.16, -0.09, -1.39 ,1.09, 0.02, -0.06};
    //    const std::vector<float> rightShoulderSeed_ = {-0.28, 0.99, 0.12 , 1.49, 1.03, 0.0, 0.0};
    CartesianPlanner* right_arm_planner_;
    CartesianPlanner* left_arm_planner_;
    WholebodyControlInterface* wholebody_controller_;
    ChestControlInterface * chest_controller_;

public:

    bool pressButton(const RobotSide side, geometry_msgs::Point &goal, float executionTime=2.0f);
    geometry_msgs::QuaternionStamped leftHandOrientation() const;
    geometry_msgs::QuaternionStamped rightHandOrientation() const;
    void getButtonPosition( geometry_msgs::Point &goal);
    ButtonPress(ros::NodeHandle &);
    ~ButtonPress();

};

#endif // BUTTON_PRESS_H
