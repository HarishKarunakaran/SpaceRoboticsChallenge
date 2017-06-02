#include "val_task3/door_opener.h"


doorOpener::doorOpener(ros::NodeHandle nh)
    :nh_(nh),armTraj_(nh_),gripper_(nh_), task3_(nh_), walker_(nh){

    robot_state_ = RobotStateInformer::getRobotStateInformer(nh_);
}

doorOpener::~doorOpener(){

}

void doorOpener::openDoor(geometry_msgs::Pose valveCenter){

    //Alligning relative to the centre of the valve
    geometry_msgs::Pose   pelvisPose;
    geometry_msgs::Pose2D preDoorOpenGoal;
    std::vector<float> x_offset, y_offset;

    robot_state_->transformPose(valveCenter,valveCenter,
                                VAL_COMMON_NAMES::WORLD_TF,VAL_COMMON_NAMES::PELVIS_TF);
    robot_state_->getCurrentPose(VAL_COMMON_NAMES::PELVIS_TF, pelvisPose);

    valveCenter.position.x  -= 0.3;

    robot_state_->transformPose(valveCenter, valveCenter, VAL_COMMON_NAMES::PELVIS_TF);

    preDoorOpenGoal.x        = valveCenter.position.x;
    preDoorOpenGoal.y        = valveCenter.position.y;
    preDoorOpenGoal.theta    = tf::getYaw(pelvisPose.orientation);
    walker_.walkToGoal(preDoorOpenGoal);

    gripper_.closeGripper(LEFT);
    gripper_.closeGripper(RIGHT);

//    std::vector<float> y_offset={0.2,0.2,0.30,0.30};
//    std::vector<float> x_offset={0.0,0.0,0.0,0.0};
//   walker_.walkLocalPreComputedSteps(x_offset,y_offset,LEFT);

//    ros::Duration(2.0).sleep();
    task3_.beforDoorOpenPose();
    ros::Duration(0.5).sleep();

     //Walk straight few small steps
    x_offset = {0.3,0.3,0.6,0.6};
    y_offset = {0.0,0.0,0,0};
    walker_.walkLocalPreComputedSteps(x_offset,y_offset,RIGHT);
    ros::Duration(1.0).sleep();
    x_offset = {0.3,0.3};
    y_offset = {0.0, 0.0};
    walker_.walkLocalPreComputedSteps(x_offset,y_offset,LEFT);
    ros::Duration(1.0).sleep();

}
