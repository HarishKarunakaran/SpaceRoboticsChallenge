#include <val_task2/solar_panel_grasp.h>
#include <stdlib.h>
#include <stdio.h>
#include "val_task_common/val_task_common_utils.h"
#include "val_control_common/val_control_common.h"

solar_panel_handle_grabber::solar_panel_handle_grabber(ros::NodeHandle n):nh_(n), armTraj_(nh_), gripper_(nh_)
{
    current_state_ = RobotStateInformer::getRobotStateInformer(nh_);
    // cartesian planners for the arm
    left_arm_planner_ = new cartesianPlanner("leftMiddleFingerGroup", VAL_COMMON_NAMES::WORLD_TF);
    right_arm_planner_ = new cartesianPlanner("rightMiddleFingerGroup", VAL_COMMON_NAMES::WORLD_TF);
    wholebody_controller_ = new wholebodyManipulation(nh_);
}

solar_panel_handle_grabber::~solar_panel_handle_grabber()
{
    delete left_arm_planner_;
    delete right_arm_planner_;
    delete wholebody_controller_;
}


bool solar_panel_handle_grabber::grasp_handles(armSide side, const geometry_msgs::Pose &goal, float executionTime)
{

    const std::vector<float>* seed;
    std::string endEffectorFrame;
    float palmToFingerOffset;
    if(side == armSide::LEFT){
        seed = &leftShoulderSeed_;
        endEffectorFrame = VAL_COMMON_NAMES::L_END_EFFECTOR_FRAME;
        palmToFingerOffset = 0.07;
    }
    else {
        seed = &rightShoulderSeed_;
        endEffectorFrame = VAL_COMMON_NAMES::R_END_EFFECTOR_FRAME;
        palmToFingerOffset = -0.07;
    }
    valControlCommon control_util(nh_);

    ROS_INFO("solar_panel_handle_grabber::grasp_handles : opening grippers");
    gripper_.controlGripper(side, GRIPPER_STATE::OPEN_THUMB_IN_APPROACH);

    //move shoulder roll outwards
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Setting shoulder roll");
    std::vector< std::vector<float> > armData;
    armData.push_back(*seed);

    //    armTraj_.moveArmJoints(side, armData, executionTime);
    //    ros::Duration(executionTime*2).sleep();
    //    control_util.stopAllTrajectories();

    //move arm to given point with known orientation and higher z
    geometry_msgs::Pose finalGoal, intermGoal;

    current_state_->transformPose(goal,intermGoal, VAL_COMMON_NAMES::WORLD_TF, VAL_COMMON_NAMES::PELVIS_TF);
    float yaw = tf::getYaw(intermGoal.orientation);
    intermGoal.position.x -= 0.2*cos(yaw);
    intermGoal.position.y -= 0.2*sin(yaw);

    //transform that point back to world frame
    current_state_->transformPose(intermGoal, intermGoal, VAL_COMMON_NAMES::PELVIS_TF, VAL_COMMON_NAMES::WORLD_TF);
    taskCommonUtils::fixHandFramePalmUp(nh_, side, intermGoal);

    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Moving at an intermidate point before goal");
    ROS_INFO("Intermidiate goal x:%f y:%f z:%f quat x:%f y:%f z:%f w:%f",intermGoal.position.x, intermGoal.position.y, intermGoal.position.z,
             intermGoal.orientation.x, intermGoal.orientation.y, intermGoal.orientation.z, intermGoal.orientation.w);
    armTraj_.moveArmInTaskSpace(side, intermGoal, executionTime*2);
    ros::Duration(executionTime*2).sleep();
    control_util.stopAllTrajectories();

    //move arm to final position with known orientation

    current_state_->transformPose(goal,finalGoal, VAL_COMMON_NAMES::WORLD_TF, VAL_COMMON_NAMES::PELVIS_TF);
    finalGoal.position.x += 0.1*cos(yaw);
    finalGoal.position.y += 0.1*sin(yaw);

    //transform that point back to world frame
    current_state_->transformPose(finalGoal, finalGoal, VAL_COMMON_NAMES::PELVIS_TF, VAL_COMMON_NAMES::WORLD_TF);

    taskCommonUtils::fixHandFramePalmUp(nh_, side, finalGoal);
    intermGoal.position.z -= 0.06;
    finalGoal.position.z  -= 0.06;
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Moving towards goal");
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Final goal  x:%f y:%f z:%f quat x:%f y:%f z:%f w:%f",finalGoal.position.x, finalGoal.position.y,
             finalGoal.position.z, finalGoal.orientation.x, finalGoal.orientation.y, finalGoal.orientation.z, finalGoal.orientation.w);
    std::vector<geometry_msgs::Pose> waypoints;

    waypoints.push_back(intermGoal);
    waypoints.push_back(finalGoal);

    finalGoal.position.z  -= 0.01;
    waypoints.push_back(finalGoal);
    moveit_msgs::RobotTrajectory traj;
    if(side == armSide::LEFT)
    {
        if (left_arm_planner_->getTrajFromCartPoints(waypoints, traj, false) < 0.98){
            ROS_INFO("solar_panel_handle_grabber::grasp_handles : Trajectory is not planned 100% - retrying");
            return false;
        }
    }
    else
    {
        if (right_arm_planner_->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
            ROS_INFO("solar_panel_handle_grabber::grasp_handles : Trajectory is not planned 100% - retrying");
            return false;
        }
    }
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Calculated Traj");
    wholebody_controller_->compileMsg(side, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();

    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Closing grippers");
    gripper_.controlGripper(side,GRIPPER_STATE::CUP);
    ros::Duration(0.3).sleep();

    finalGoal.position.z  += 0.2;
    waypoints.clear();
    waypoints.push_back(finalGoal);

    moveit_msgs::RobotTrajectory traj2;
    if(side == armSide::LEFT)
    {
        if (left_arm_planner_->getTrajFromCartPoints(waypoints, traj2, false) < 0.98){
            ROS_INFO("solar_panel_handle_grabber::grasp_handles : Trajectory is not planned 100% - retrying");
            return false;
        }
    }
    else
    {
        if (right_arm_planner_->getTrajFromCartPoints(waypoints, traj2, false)< 0.98){
            ROS_INFO("solar_panel_handle_grabber::grasp_handles : Trajectory is not planned 100% - retrying");
            return false;
        }
    }

    geometry_msgs::Pose finalFramePose;

    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Fecthing position of %s", endEffectorFrame.c_str());
    current_state_->getCurrentPose(endEffectorFrame,finalFramePose, VAL_COMMON_NAMES::WORLD_TF);

    float x_diff = (finalGoal.position.x - finalFramePose.position.x);
    float y_diff = (finalGoal.position.y - finalFramePose.position.y);
    float z_diff = (finalGoal.position.z - finalFramePose.position.z);

    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Expected Goal Pose : %f %f %f", finalGoal.position.x, finalGoal.position.y, finalGoal.position.z);
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Actual Finger Pose : %f %f %f", finalFramePose.position.x, finalFramePose.position.y, finalFramePose.position.z);
    ROS_INFO("solar_panel_handle_grabber::grasp_handles : Distance between final pose and goal is %f", sqrt(x_diff*x_diff + y_diff*y_diff + z_diff*z_diff));

    //    if (sqrt(x_diff*x_diff + y_diff*y_diff + z_diff*z_diff) > 0.1){
    //        return false;
    //    }

    return true;
}
