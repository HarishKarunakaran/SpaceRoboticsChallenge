#include <ros/ros.h>
#include <val_control/val_arm_navigation_.h>
#include <val_control/val_pelvis_navigation.h>
#include <val_footstep/ValkyrieWalker.h>
#include <ihmc_msgs/ChestTrajectoryRosMessage.h>
#include <ihmc_msgs/FootTrajectoryRosMessage.h>
#include <tf2/utils.h>

enum sm {
    PREPARE_START = 0,
    WALK_TO_DOOR,
    EXIT
};

int main(int argc, char** argv)
{
    ros::init(argc, argv, "Qual2");
    ros::NodeHandle nh;


    armTrajectory armtraj(nh);
    pelvisTrajectory pelvisTraj(nh);
    chestTrajectory chestTraj(nh);

    float swingTime=0.45, transferTime=0.35, swingHeight=0.13, pelvisHeight=1.07;
    if ( argc != 5 ) // 5 arguments required
    {
        std::cout<<"The values are not set, using the default values for run..!!!!!"<< std::endl;
        std::cout<<"transferTime: "<<transferTime<<"\n"<<"swingTime: "<<swingTime<<"\n"<<"swingHeight: "<<swingHeight<<"\n"<<"pelvisHeight: "<<pelvisHeight<<"\n";
    }
    else
    {
        transferTime = std::atof(argv[1]);
        swingTime = std::atof(argv[2]);
        swingHeight = std::atof(argv[3]);
        pelvisHeight = std::atof(argv[4]);
    }

    // optimum values for walking
    //ValkyrieWalker walk(nh, 0.34, 0.44);

    ValkyrieWalker walk(nh, transferTime, swingTime);
    walk.setSwing_height(swingHeight);

    sm state = PREPARE_START;
    // The code structure is so to get the fastest and optimum execution
    switch (state)
    {
    case PREPARE_START:
    {
        pelvisTraj.controlPelvisHeight(pelvisHeight);

       // walk.WalkNStepsForward(11,0.35,0,false,RIGHT);
        ros::Duration(0.5).sleep();
        ///get the armtrajectory message from the arm_trajectory*.cpp
        /// and modify the hardcoded part from buttonpress function and publish the message
        armtraj.buttonPressPrepareArm(RIGHT);
        state = WALK_TO_DOOR;

        // fall through the case, so we save one iteration (0.1 sec)
        //break;
    }
    case WALK_TO_DOOR:
    {
        walk.setWalkParms(transferTime, swingTime, 0);
        ///get the footsteplist message and modify the footsteps based on
        /// what is hardcoded in ValkyrieWalker.cpp and publish it again

        walk.WalkNStepsForward(12,0.51,0, false, RIGHT);
        break;
    }

    default:
EXIT:
        break;
    }

    ros::spinOnce();
    return 0;
}


bool OrientChest(float roll, float pitch, float yaw, ros::Publisher pub){
    ihmc_msgs::ChestTrajectoryRosMessage msg;
    std::vector<ihmc_msgs::SO3TrajectoryPointRosMessage> trajPointsVec;
    ihmc_msgs::SO3TrajectoryPointRosMessage trajPoint1, trajPoint2;
    tf::Quaternion angles;
    angles.setRPY((tfScalar)roll*3.1427/180, (tfScalar)pitch*3.1427/180, (tfScalar)yaw*3.1427/180);

    geometry_msgs::Quaternion angles1;
    angles1.x = angles.getX();
    angles1.y = angles.getY();
    angles1.z = angles.getZ();
    angles1.w = angles.getW();

    trajPoint1.orientation = angles1;
    trajPoint1.time = 3.0;
    trajPointsVec.push_back(trajPoint1);

    geometry_msgs::Quaternion angles2;
    angles2.x = 0.0;
    angles2.y = 0.0;
    angles2.z = 0.0;
    angles2.w = 1.0;

    trajPoint2.orientation = angles2;
    trajPoint2.time = 0.5;
    trajPointsVec.push_back(trajPoint2);


    msg.execution_mode = 0;
    msg.unique_id = 13;
    msg.taskspace_trajectory_points = trajPointsVec;

    pub.publish(msg);
    //ROS_INFO("Published chest messages");

}

//void footStepTraj(ros::Publisher pub)
//{
//    ihmc_msgs::FootTrajectoryRosMessage msgR;
//    ihmc_msgs::FootTrajectoryRosMessage msgL;
//    ihmc_msgs::SE3TrajectoryPointRosMessage trajPointR, trajPointL;

//    msgR.execution_mode=0;
//    msgR.robot_side = RIGHT;
//    msgR.previous_message_id = 0;
//    msgR.taskspace_trajectory_points.clear();
//    msgR.unique_id = 40;

//    trajPointR.unique_id = 41; //(int)ros::Time::now();
//    trajPointR.time = 0.0; // reach instantenously
//    trajPointR.position.x = 0.0;
//    trajPointR.position.y = -0.0626366231343;
//    trajPointR.position.z = 0.0;
//    trajPointR.orientation.x = -1.16587645216e-06;
//    trajPointR.orientation.y = 3.03336607205e-06;
//    trajPointR.orientation.z = 0.000763307697428;
//    trajPointR.orientation.w = 0.999999708675;
//    trajPointR.time = 0.0;
//    msgR.taskspace_trajectory_points.push_back(trajPointR);

//    trajPointR.unique_id = 42; //(int)ros::Time::now();
//    trajPointR.position.x = 0.4;
//    trajPointR.position.y = -0.0626366231343;
//    trajPointR.position.z = 0.0;
//    trajPointR.orientation.x = -1.16587645216e-06;
//    trajPointR.orientation.y = 3.03336607205e-06;
//    trajPointR.orientation.z = 0.000763307697428;
//    trajPointR.orientation.w = 0.999999708675;
//    trajPointR.time = 0.8;
//    msgR.taskspace_trajectory_points.push_back(trajPointR);

//    trajPointR.unique_id = 43; //(int)ros::Time::now();
//    trajPointR.position.x = 0.4;
//    trajPointR.position.y = -0.0626366231343;
//    trajPointR.position.z = 0.0;
//    trajPointR.orientation.x = -1.16587645216e-06;
//    trajPointR.orientation.y = 3.03336607205e-06;
//    trajPointR.orientation.z = 0.000763307697428;
//    trajPointR.orientation.w = 0.999999708675;
//    trajPointR.time = 3.1;
//    msgR.taskspace_trajectory_points.push_back(trajPointR);


//    msgL.execution_mode=0;
//    msgL.robot_side = LEFT;
//    msgL.previous_message_id = 0;
//    msgL.taskspace_trajectory_points.clear();
//    msgL.unique_id = 4;

//    trajPointL.unique_id = 12; //(int)ros::Time::now();
//    trajPointL.time = 0.4; // reach instantenously
//    trajPointL.position.x = 0.0;
//    trajPointL.position.y = 0.118900228121;
//    trajPointL.position.z = 0.0;
//    trajPointL.orientation.x =  9.79238581203e-06;
//    trajPointL.orientation.y = 1.5494878979e-05;
//    trajPointL.orientation.z = -0.00103408687569;
//    trajPointL.orientation.w =  0.999999465164;

//    msgL.taskspace_trajectory_points.push_back(trajPointL);

//    trajPointL.unique_id = 11; //(int)ros::Time::now();
//    trajPointL.position.x = 0.4;
//    trajPointL.position.y = 0.118900228121;
//    trajPointL.position.z = 0.0;
//    trajPointL.orientation.x =  9.79238581203e-06;
//    trajPointL.orientation.y = 1.5494878979e-05;
//    trajPointL.orientation.z = -0.00103408687569;
//    trajPointL.orientation.w =  0.999999465164;
//    trajPointL.time = 1.6; // reach instantenously

//    msgL.taskspace_trajectory_points.push_back(trajPointL);


//   // ROS_INFO("published msg");
//    pub.publish(msgR);
//    ros::Duration(0.25).sleep();
//    pub.publish(msgL);
//}
