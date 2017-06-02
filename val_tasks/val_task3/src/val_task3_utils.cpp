#include <val_task3/val_task3_utils.h>

task3Utils::task3Utils(ros::NodeHandle nh): nh_(nh),arm_controller_(nh_) {

}

task3Utils::~task3Utils(){

}

void task3Utils::beforePanelManipPose(){

   std::vector< std::vector<float> > armData;
   armData.push_back(RIGHT_ARM_SEED_TABLE_MANIP);

    arm_controller_.moveArmJoints(RIGHT,armData,2.0f);
    ros::Duration(2.0).sleep();

    armData.clear();
    armData.push_back(LEFT_ARM_SEED_TABLE_MANIP);
    arm_controller_.moveArmJoints(LEFT,armData,2.0f);
    ros::Duration(2.0).sleep();
}

void task3Utils::beforDoorOpenPose(){


    std::vector< std::vector<float> > armData;
    armData.push_back(RIGHT_ARM_DOOR_OPEN);

     arm_controller_.moveArmJoints(RIGHT,armData,2.0f);
     ros::Duration(2.0).sleep();

     armData.clear();
     armData.push_back(LEFT_ARM_DOOR_OPEN);
     arm_controller_.moveArmJoints(LEFT,armData,2.0f);
     ros::Duration(2.0).sleep();
}
