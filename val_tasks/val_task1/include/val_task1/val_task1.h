#pragma once

// !!!!! important other wise created collision with std
#define DISABLE_DECISION_MAKING_LOG true

#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

#include <decision_making/FSM.h>
#include <decision_making/ROSTask.h>
#include <decision_making/DecisionMaking.h>

#include <ros/ros.h>
#include <geometry_msgs/Pose2D.h>
#include <nav_msgs/OccupancyGrid.h>

#include "val_footstep/ValkyrieWalker.h"
#include "val_task_common/val_walk_tracker.h"
#include "val_task_common/panel_detection.h"
#include "val_controllers/val_chest_navigation.h"
#include "val_controllers/val_pelvis_navigation.h"
#include "val_controllers/val_head_navigation.h"
#include "val_controllers/val_gripper_control.h"
#include "val_controllers/val_arm_navigation.h"
#include "val_task1/handle_detector.h"
#include "val_task1/handle_grabber.h"
#include "val_controllers/robot_state.h"
#include "val_task1/move_handle.h"
#include "val_task_common/val_task_common_utils.h"
#include "val_task_common/finish_box_detector.h"
#include <val_task1/val_task1_utils.h>
#include <val_moveit_planners/val_cartesian_planner.h>
#include <val_controllers/val_wholebody_manipulation.h>
#include <val_task_common/val_upperbody_tracker.h>
#include <val_control_common/val_control_common.h>
#include <val_task1/pcl_handle_detector.h>
#include <chrono>
#include <ctime>

using namespace decision_making;

#define foreach BOOST_FOREACH

FSM_HEADER(val_task1)

class valTask1 {
    private:
    ros::NodeHandle nh_;
    //private contructor. use getValTask1() to create an object
    valTask1(ros::NodeHandle nh);

    // object for the walker api
    ValkyrieWalker* walker_;
    // object for tracking robot walk
    walkTracking* walk_track_;

    // panel detection object
    PanelDetector* panel_detector_;
    //handle detector
    HandleDetector* handle_detector_;
    //moving handle
    move_handle* move_handle_;
    // Object to use for grasping handles
    handle_grabber* handle_grabber_;
    //Finish box detector
    FinishBoxDetector* finish_box_detector_;

    // chest controller
    chestTrajectory* chest_controller_;
    //pelvis controller
    pelvisTrajectory* pelvis_controller_;
    //head controller
    HeadTrajectory* head_controller_;
    //grippers
    gripperControl* gripper_controller_;
    // arm
    armTrajectory* arm_controller_;
    // whole body controller
    wholebodyManipulation* wholebody_controller_;
    //robot state informer
    RobotStateInformer* robot_state_;
    // task1 utils
    task1Utils* task1_utils_;
    // cartesian planner
    cartesianPlanner* right_arm_planner_;
    cartesianPlanner* left_arm_planner_;
    // grasp state variable
    prevGraspState prev_grasp_state_;
    // val control common api's
    valControlCommon* control_helper_;
    // pcl handle detector for redetcting
    pcl_handle_detector* pcl_handle_detector_;

    ros::Publisher array_pub_;

    //required for initialization. Move out init state only if map is updated twice.
    ros::Subscriber occupancy_grid_sub_;
    unsigned int map_update_count_;
    void occupancy_grid_cb(const nav_msgs::OccupancyGrid::Ptr msg);

    // this pointer is to ensure that only 1 object of this task is created.
    static valTask1 *currentObject;

    // handle pos and center
    std::vector<geometry_msgs::Point> handle_loc_;

    // panel plane coeffecients
    std::vector<float> panel_coeff_;

    // Visited map
    ros::Subscriber visited_map_sub_;
    void visited_map_cb(const nav_msgs::OccupancyGrid::Ptr msg);
    nav_msgs::OccupancyGrid visited_map_;

    // goal location for the finish box
    geometry_msgs::Pose2D next_finishbox_center_;

    // helper functions
    void resetRobotToDefaults(int arm_pose=1);

    public:

    // goal location for the panel
    geometry_msgs::Pose2D panel_walk_goal_coarse_;
    geometry_msgs::Pose2D panel_walk_goal_fine_;

    // default constructor and destructor
    static valTask1* getValTask1(ros::NodeHandle nh);
    ~valTask1();

    bool preemptiveWait(double ms, decision_making::EventQueue& queue);
    decision_making::TaskResult initTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult detectPanelCoarseTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult walkToSeePanelTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult detectHandleCenterTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult detectPanelFineTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult walkToPanel(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult graspPitchHandleTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult controlPitchTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult graspYawHandleTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult controlYawTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult redetectHandleTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult detectfinishBoxTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult walkToFinishTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult endTask(string name, const FSMCallContext& context, EventQueue& eventQueue);
    decision_making::TaskResult errorTask(string name, const FSMCallContext& context, EventQueue& eventQueue);

    geometry_msgs::Pose2D getPanelWalkGoal();
    void setPanelWalkGoal(const geometry_msgs::Pose2D &panel_walk_goal_coarse_);
    void setPanelWalkGoalFine(const geometry_msgs::Pose2D &panel_walk_goal);
    void setPanelCoeff(const std::vector<float> &panel_coeff);
    void setFinishboxGoal (const geometry_msgs::Pose2D &next_finishbox_center);
};
