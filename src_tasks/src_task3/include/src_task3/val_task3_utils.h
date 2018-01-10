#ifndef VAL_TASK3_UTILS_H
#define VAL_TASK3_UTILS_H

#include <ros/ros.h>
#include <srcsim/Satellite.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Pose2D.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <tough_common/val_common_names.h>
#include <tough_controller_interface/chest_control_interface.h>
#include <tough_controller_interface/pelvis_control_interface.h>
#include <tough_controller_interface/gripper_control_interface.h>
#include <tough_controller_interface/head_control_interface.h>
#include <tough_controller_interface/arm_control_interface.h>
#include <tough_controller_interface/robot_state.h>
#include <tough_footstep/RobotWalker.h>
#include "navigation_common/map_generator.h"
#include <srcsim/Task.h>
#include <mutex>
#include <std_msgs/String.h>

class task3Utils{
private:

    const std::string TEXT_RED="\033[0;31m";
    const std::string TEXT_GREEN = "\033[0;32m";
    const std::string TEXT_NC=  "\033[0m";

    ros::NodeHandle nh_;
    ArmControlInterface arm_controller_;
    srcsim::Task task_msg_;

    bool is_climbstairs_finished_;
    std::mutex climstairs_flag_mtx_;

    const std::vector<float> RIGHT_ARM_SEED_TABLE_MANIP = {-0.23, 0, 0.70, 1.51, -0.05, 0, 0};
    const std::vector<float> LEFT_ARM_SEED_TABLE_MANIP  = {-0.23, 0, 0.70, -1.51, 0.05, 0, 0};
    const std::vector<float> RIGHT_ARM_DOOR_OPEN        = {-1.35, 1.20, 0.75, 0.50, 1.28, 0.0, 0.0};
    const std::vector<float> LEFT_ARM_DOOR_OPEN         = {-0.2f, -1.2f, 0.7222f, -1.5101f, 0.0f, 0.0f, 0.0f};
    const std::vector<float> RIGHT_ARM_DOORWAY_WALK     = {-0.23, 1.51, 0.28, 1.49, 1.28, 0, 0};
    const std::vector<float> LEFT_ARM_DOORWAY_WALK      = {-0.23, -1.51, 0.33, -1.53, 1.24, 0, 0};


    RobotStateInformer *current_state_;
    RobotDescription *rd_;
    PelvisControlInterface* pelvis_controller_;
    RobotWalker *walk_;

    // Visited map
    ros::Subscriber visited_map_sub_, task_status_sub_;
    ros::Publisher task3_log_pub_ ;
    void visited_map_cb(const nav_msgs::OccupancyGrid::Ptr msg);
    void taskStatusCB(const srcsim::Task &msg);
    nav_msgs::OccupancyGrid visited_map_;
    int current_checkpoint_;
public:
    task3Utils(ros::NodeHandle nh);
    ~task3Utils();
    void beforePanelManipPose();
    void beforDoorOpenPose();
    void doorWalkwayPose();
    void blindNavigation(geometry_msgs::Pose2D &goal);
    geometry_msgs::Pose grasping_hand(RobotSide &side, geometry_msgs::Pose handle_pose);

    bool isClimbstairsFinished() const;
    void resetClimbstairsFlag(bool);
    void resetClimbstairsFlag(void);
    void task3LogPub(std::string data);

    void walkSideways(float error);
};






















#endif // VAL_TASK3_UTILS_H
