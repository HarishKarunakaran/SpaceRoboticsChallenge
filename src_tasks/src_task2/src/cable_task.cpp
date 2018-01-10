#include <src_task2/cable_task.h>


CableTask::CableTask(ros::NodeHandle n):nh_(n), armTraj_(nh_), gripper_(nh_)
{
    current_state_ = RobotStateInformer::getRobotStateInformer(nh_);
    rd_ = RobotDescription::getRobotDescription(nh_);

    /* Top Grip Flat Hand modified*/
    rightHandOrientationTop_.header.frame_id = rd_->getPelvisFrame();
    rightHandOrientationTop_.quaternion.x = -0.430;
    rightHandOrientationTop_.quaternion.y = 0.541;
    rightHandOrientationTop_.quaternion.z = 0.481;
    rightHandOrientationTop_.quaternion.w = 0.539;

    /* Angled Grip Flat Hand modified*/
    rightHandOrientationAngle_.header.frame_id = rd_->getPelvisFrame();
    rightHandOrientationAngle_.quaternion.x = -0.206;
    rightHandOrientationAngle_.quaternion.y = 0.672;
    rightHandOrientationAngle_.quaternion.z = 0.413;
    rightHandOrientationAngle_.quaternion.w = 0.579;


    quat.header.frame_id = rd_->getPelvisFrame();
    quat.quaternion.x = 0.124;
    quat.quaternion.y = -0.232;
    quat.quaternion.z = 0.964;
    quat.quaternion.w = 0.039;


    // cartesian planners for the arm
    right_arm_planner_choke = new CartesianPlanner(VAL_COMMON_NAMES::RIGHT_ENDEFFECTOR_GROUP, VAL_COMMON_NAMES::WORLD_TF);
    right_arm_planner_cable = new CartesianPlanner(VAL_COMMON_NAMES::RIGHT_ENDEFFECTOR_GROUP, VAL_COMMON_NAMES::WORLD_TF);

    left_arm_planner_choke = new CartesianPlanner(VAL_COMMON_NAMES::LEFT_ENDEFFECTOR_GROUP, VAL_COMMON_NAMES::WORLD_TF);
    left_arm_planner_cable = new CartesianPlanner(VAL_COMMON_NAMES::LEFT_ENDEFFECTOR_GROUP, VAL_COMMON_NAMES::WORLD_TF);

    wholebody_controller_ = new WholebodyControlInterface(nh_);
    chest_controller_     = new ChestControlInterface(nh_);

    task2_utils_    = new task2Utils(nh_);
    control_common_ = new ToughControlCommon(nh_);

}

CableTask::~CableTask()
{
    delete right_arm_planner_choke;
    delete right_arm_planner_cable;
    delete left_arm_planner_choke;
    delete left_arm_planner_cable;
    delete wholebody_controller_;
    delete chest_controller_;
}


bool CableTask::grasp_choke(RobotSide side, const geometry_msgs::Pose &goal, float executionTime)
{

    ToughControlCommon control_util(nh_);

    // Setting gripper positions
    ROS_INFO("CableTask::grasp_choke : opening grippers");
    gripper_.controlGripper(side, GRIPPER_STATE::OPEN);

    // Initial configurations are common for both sides
    ROS_INFO("CableTask::grasp_choke : Setting initial positions");

    std::vector< std::vector<float> > armData;
    if(side ==LEFT){
        armData.clear();
        armData.push_back(leftShoulderSeedInitial_);
        armTraj_.moveArmJoints(LEFT, armData, executionTime);
        ros::Duration(executionTime).sleep();
    }
    else
    {
        armData.clear();
        armData.push_back(rightShoulderSeedInitial_);
        armTraj_.moveArmJoints(RIGHT, armData, executionTime);
        ros::Duration(executionTime).sleep();
    }


    //move arm to given point with known orientation and higher z
    geometry_msgs::Pose finalGoal, intermGoal;

    float offset =0.1;
    intermGoal= goal;
    intermGoal.position.z+=offset;

    // fixing the flat palm orientation
    taskCommonUtils::fixHandFramePalmDown(nh_, side, intermGoal);

    ROS_INFO("CableTask::grasp_choke : Moving at an intermidate point before goal");
    ROS_INFO("CableTask::grasp_choke : Intermidiate goal x:%f y:%f z:%f quat x:%f y:%f z:%f w:%f",intermGoal.position.x, intermGoal.position.y, intermGoal.position.z,
             intermGoal.orientation.x, intermGoal.orientation.y, intermGoal.orientation.z, intermGoal.orientation.w);

    // setting a sead position without movement of chest
    armTraj_.moveArmInTaskSpace(side, intermGoal, executionTime);
    ros::Duration(executionTime*2).sleep();
    control_util.stopAllTrajectories();

    finalGoal = goal;
    finalGoal.position.z-=0.01;
    taskCommonUtils::fixHandFramePalmDown(nh_, side, finalGoal);

    ROS_INFO("CableTask::grasp_choke: Moving towards goal");
    ROS_INFO("CableTask::grasp_choke: Final goal  x:%f y:%f z:%f quat x:%f y:%f z:%f w:%f",finalGoal.position.x, finalGoal.position.y,
             finalGoal.position.z, finalGoal.orientation.x, finalGoal.orientation.y, finalGoal.orientation.z, finalGoal.orientation.w);
    std::vector<geometry_msgs::Pose> waypoints;

    waypoints.push_back(intermGoal);
    waypoints.push_back(finalGoal);

    moveit_msgs::RobotTrajectory traj;


    if(side ==RIGHT)
    {
        if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
            ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
            armData.clear();
            armData.push_back(rightShoulderSeedInitial_);
            armTraj_.moveArmJoints(RIGHT, armData, executionTime);
            ros::Duration(executionTime).sleep();
            return false;
        }
    }
    else
    {
        if (left_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
            ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
            armData.clear();
            armData.push_back(leftShoulderSeedInitial_);
            armTraj_.moveArmJoints(LEFT, armData, executionTime);
            ros::Duration(executionTime).sleep();

            return false;
        }
    }


    ROS_INFO("CableTask::grasp_choke: Calculated Traj");
    wholebody_controller_->executeTrajectory(side, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();

    //    geometry_msgs::Pose finalFramePose;
    //    ROS_INFO("CableTask::grasp_choke: Fecthing position of %s", endEffectorFrame.c_str());
    //    current_state_->getCurrentPose(endEffectorFrame,finalFramePose, VAL_COMMON_NAMES::WORLD_TF);

    //    float x_diff = (finalGoal.position.x - finalFramePose.position.x);
    //    float y_diff = (finalGoal.position.y - finalFramePose.position.y);
    //    float z_diff = (finalGoal.position.z - finalFramePose.position.z);

    //    ROS_INFO("CableTask::grasp_choke: Expected Goal Pose : %f %f %f", finalGoal.position.x, finalGoal.position.y, finalGoal.position.z);
    //    ROS_INFO("CableTask::grasp_choke: Actual Finger Pose : %f %f %f", finalFramePose.position.x, finalFramePose.position.y, finalFramePose.position.z);
    //    ROS_INFO("CableTask::grasp_choke: Distance between final pose and goal is %f", sqrt(x_diff*x_diff + y_diff*y_diff + z_diff*z_diff));

    //    if (sqrt(x_diff*x_diff + y_diff*y_diff + z_diff*z_diff) > 0.1){
    //        return false;
    //    }

    std::vector<double> openGrip={0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> closeGrip={1.3999, 0.55, 1.1, 0.9, 1.0};
    taskCommonUtils::slowGrip(nh_,side,openGrip,closeGrip);
    ros::Duration(0.3).sleep();
    gripper_.closeGripper(side);
    ros::Duration(0.3).sleep();

    // Setting arm position to dock
    ROS_INFO("grasp_cable: Setting arm position to dock cable");
    armData.clear();
    armData.push_back(rightAfterGraspShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT,armData,executionTime);
    ros::Duration(0.3).sleep();

    chest_controller_->controlChest(0,0,0);
    ros::Duration(1).sleep();
    return true;
}

bool CableTask::grasp_cable(const geometry_msgs::Pose &goal, float executionTime)
{
    /* set orientation desired ( need to get this from perception ) @Todo change later!
     * set seed point. one standard initial point to avoid collision and the other closer to the cable
     * set grasping pose. approach from the thumb in the bottom. close all fingers to grasp
     * set last seed position based on how to approach the socket
     */

    // Setting initial positions for both arms
    ROS_INFO("grasp_cable: Initial Pose");
    std::vector< std::vector<float> > armData;
    armData.push_back(leftShoulderSeedInitial_);
    armTraj_.moveArmJoints(LEFT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    armData.clear();
    armData.push_back(rightShoulderSeedInitial_);
    armData.push_back(rightShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    geometry_msgs::Pose rightOffset;
    current_state_->getCurrentPose("/rightMiddleFingerPitch1Link",rightOffset,"/rightThumbRollLink");
    geometry_msgs::Pose intermGoal;
    //    intermGoal=goal;
    current_state_->transformPose(goal,intermGoal, VAL_COMMON_NAMES::WORLD_TF, rd_->getRightEEFrame());
    intermGoal.position.x+=rightOffset.position.x;
    intermGoal.position.y+=rightOffset.position.y;
    intermGoal.position.z+=rightOffset.position.z;
    current_state_->transformPose(intermGoal,intermGoal, rd_->getRightEEFrame(), VAL_COMMON_NAMES::WORLD_TF);

    taskCommonUtils::fixHandFramePalmDown(nh_, RIGHT, intermGoal);

    float offset=0.03;
    geometry_msgs::Pose final;
    final=intermGoal;
    final.position.z+=(2*offset);

    std::vector<geometry_msgs::Pose> waypoints;
    waypoints.push_back(final);

    final=intermGoal;
    final.position.z+=(offset/3);
    waypoints.push_back(final);

    moveit_msgs::RobotTrajectory traj;

    // Sending waypoints to the planner
    right_arm_planner_cable->getTrajFromCartPoints(waypoints,traj,false);

    // Opening grippers
    ROS_INFO("grasp_cable: Setting gripper position to open thumb");
    ROS_INFO("opening grippers");
    std::vector<double> gripper0,gripper1,gripper2,gripper3;
    gripper0={1.35, 1.35, 0.3, 0.0 ,0.0 };
    gripper1={1.2, 0.4, 0.35, 0.0 ,0.0 };
    gripper2={1.2, 0.6, 0.7, 0.0 ,0.0 };
    gripper3={1.2, 0.6, 0.7, 0.9 ,1.0 };
    gripper_.controlGripper(RIGHT,gripper0);
    ros::Duration(0.1).sleep();
    gripper_.controlGripper(RIGHT, gripper1);
    ros::Duration(0.2).sleep();


    // Planning whole body motion
    wholebody_controller_->executeTrajectory(RIGHT,traj.joint_trajectory);
    ros::Duration(executionTime).sleep();

    //    ROS_INFO("Grip Sequence 1");
    //    gripper_.controlGripper(RIGHT, gripper1);
    //    ros::Duration(0.1).sleep();
    //    ROS_INFO("Grip Sequence 2");
    //    gripper_.controlGripper(RIGHT, gripper2);
    //    ros::Duration(0.1).sleep();
    //    ROS_INFO("Grip Sequence 3");
    //    gripper_.controlGripper(RIGHT, gripper3);
    //    ros::Duration(0.1).sleep();

    taskCommonUtils::slowGrip(nh_,RIGHT,gripper1,gripper2,10,4);
    gripper_.controlGripper(RIGHT, gripper2);
    ros::Duration(0.3).sleep();
    taskCommonUtils::slowGrip(nh_,RIGHT,gripper2,gripper3,10,4);
    gripper_.controlGripper(RIGHT, gripper3);
    ros::Duration(0.3).sleep();

    // Setting arm position to dock
    ROS_INFO("grasp_cable: Setting arm position to dock cable");
    armData.clear();
    armData.push_back(rightAfterGraspShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT,armData,executionTime);
    ros::Duration(0.3).sleep();

    chest_controller_->controlChest(0,0,0);
    ros::Duration(0.3).sleep();


    // Grasping the cable
    gripper_.closeGripper(RIGHT);


}


bool CableTask::insert_cable(const geometry_msgs::Point &goal, float executionTime)
{
    const float OFFSET=0.03; // z offset of cable from middlefinger
    current_state_->transformQuaternion(quat, quat);

    std::vector<geometry_msgs::Pose> waypoints;

    for (float z=0.05; z > -0.06; z -= 0.01){
        for (float x=-0.05; x < 0.06; x+= 0.01){
            for (float y=-0.05; y < 0.06; y+= 0.01){
                geometry_msgs::Pose waypoint;
                waypoint.position.x = goal.x + x;
                waypoint.position.y = goal.y + y;
                waypoint.position.z = goal.z + z + OFFSET;
                waypoint.orientation = quat.quaternion;
                waypoints.push_back(waypoint);
            }
        }
    }

    moveit_msgs::RobotTrajectory traj;

    // Sending waypoints to the planner
    std::vector< std::vector<float> > armData;
    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
        ROS_INFO("grasp_cable: Setting arm position to dock cable");
        armData.clear();
        armData.push_back(rightAfterGraspShoulderSeed_);
        armTraj_.moveArmJoints(RIGHT,armData,executionTime);
        ros::Duration(0.3).sleep();
        return false;
    }

    wholebody_controller_->executeTrajectory(RIGHT,traj.joint_trajectory);

    ros::Time startTime= ros::Time::now();
    ros::Time endTime;
    bool cable_touching = false;
    while((ros::Time::now() - startTime) < ros::Duration(15)){
        // check effort values, but get  a threshold before doing that
        if (task2_utils_->isCableTouchingSocket()){
            cable_touching = true;
            control_common_->stopAllTrajectories();
            while(task2_utils_->isCableTouchingSocket() && task2_utils_->getCurrentCheckpoint() != 5){
                ROS_INFO("Cable is touching the socket");
            }
            endTime = ros::Time::now();
            break;
        }
    }
    // use endtime to find the traj point that we reached and reverse the trajectory
}

bool CableTask::rotate_cable1(const geometry_msgs::Pose &goal, float executionTime)
{
    // *************************goal point oriented approach

    ToughControlCommon control_util(nh_);

    // setting initial poses
    ROS_INFO("grasp_cable: Initial Pose");
    std::vector< std::vector<float> > armData;
    armData.push_back(leftShoulderSeedInitial_);
    armTraj_.moveArmJoints(LEFT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    armData.clear();
    armData.push_back(rightShoulderSeedInitial_);
    armData.push_back(rightShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    // Opening grippers
    ROS_INFO("grasp_cable: Setting gripper position to open thumb");
    ROS_INFO("opening grippers");
    std::vector<double> gripper0,gripper1,gripper2,gripper3;
    gripper0={1.35, 1.35, 0.3, 0.0 ,0.0 };
    gripper1={1.2, 0.4, 0.35, 0.3 ,0.3 };

    gripper_.controlGripper(RIGHT,gripper0);
    ros::Duration(0.1).sleep();
    gripper_.controlGripper(RIGHT, gripper1);
    ros::Duration(0.2).sleep();

    geometry_msgs::Pose rightOffset;
    current_state_->getCurrentPose("/rightMiddleFingerPitch1Link",rightOffset,"/rightThumbRollLink");
    geometry_msgs::Pose intermGoal;

    intermGoal= goal;
    taskCommonUtils::fixHandFramePalmDown(nh_, RIGHT, intermGoal);

    // setting a sead position without movement of chest
    intermGoal.position.z+=0.06;
    armTraj_.moveArmInTaskSpace(RIGHT, intermGoal, executionTime);
    ros::Duration(executionTime*2).sleep();
    control_util.stopAllTrajectories();

    geometry_msgs::Pose finalGoal;
    finalGoal = goal;
    finalGoal.position.z+=0.03;
    // change in orientation
    current_state_->transformPose(finalGoal,finalGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    finalGoal.orientation= rightHandOrientationTop_.quaternion;
    current_state_->transformPose(finalGoal,finalGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);

    std::vector<geometry_msgs::Pose> waypoints;

    waypoints.push_back(intermGoal);
    waypoints.push_back(finalGoal);

    moveit_msgs::RobotTrajectory traj;


    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
        return false;
    }


    ROS_INFO("CableTask::grasp_choke: Calculated Traj");
    wholebody_controller_->executeTrajectory(RIGHT, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();

    std::vector<double> closeGrip={1.3999, 0.55, 1.1, 0.9, 1.0};
    taskCommonUtils::slowGrip(nh_,RIGHT,gripper1,closeGrip);
    ros::Duration(0.3).sleep();
    gripper_.closeGripper(RIGHT);
    ros::Duration(0.3).sleep();

    // Setting arm position to dock
    ROS_INFO("grasp_cable: Setting arm position to dock cable");
    armData.clear();
    armData.push_back(rightAfterGraspShoulderSeed2_);
    armTraj_.moveArmJoints(RIGHT,armData,executionTime);
    ros::Duration(0.3).sleep();

    chest_controller_->controlChest(0,0,0);
    ros::Duration(0.3).sleep();

    return true;

    // *************** done
}

bool CableTask::rotate_cable2(const geometry_msgs::Pose &goal, float executionTime)
{
    // ************* open hand rotate approach

    ToughControlCommon control_util(nh_);

    // Setting gripper positions
    ROS_INFO("CableTask::grasp_choke : opening grippers");
    gripper_.controlGripper(RIGHT, GRIPPER_STATE::OPEN);

    std::vector< std::vector<float> > armData;
    armData.push_back(leftShoulderSeedInitial_);
    armTraj_.moveArmJoints(LEFT, armData, executionTime);
    ros::Duration(0.2).sleep();

    armData.clear();
    armData.push_back(rightShoulderSeedInitial_);
    armTraj_.moveArmJoints(RIGHT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    //move arm to given point with known orientation and higher z
    geometry_msgs::Pose finalGoal, intermGoal;

    float offset =0.1;
    intermGoal= goal;
    intermGoal.position.z+=offset;

    current_state_->transformPose(intermGoal,intermGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    intermGoal.orientation=rightHandOrientationAngle_.quaternion;
    current_state_->transformPose(intermGoal,intermGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);

    // setting a sead position without movement of chest
    armTraj_.moveArmInTaskSpace(RIGHT, intermGoal, executionTime);
    ros::Duration(executionTime*2).sleep();
    control_util.stopAllTrajectories();

    finalGoal = goal;
    finalGoal.position.z-=0.01;
    current_state_->transformPose(finalGoal,finalGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    finalGoal.orientation=rightHandOrientationTop_.quaternion;
    current_state_->transformPose(finalGoal,finalGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);

    std::vector<geometry_msgs::Pose> waypoints;

    waypoints.push_back(intermGoal);
    intermGoal.position.z-=(0.01+offset);
    waypoints.push_back(intermGoal);
    waypoints.push_back(finalGoal);

    moveit_msgs::RobotTrajectory traj;

    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
        return false;
    }

    ROS_INFO("CableTask::grasp_choke: Calculated Traj");
    wholebody_controller_->executeTrajectory(RIGHT, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();


    std::vector<double> openGrip={0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> closeGrip={1.3999, 0.55, 1.1, 0.9, 1.0};
    taskCommonUtils::slowGrip(nh_,RIGHT,openGrip,closeGrip);
    ros::Duration(0.3).sleep();
    gripper_.closeGripper(RIGHT);
    ros::Duration(0.3).sleep();

    // Setting arm position to dock
    ROS_INFO("grasp_cable: Setting arm position to dock cable");
    armData.clear();
    armData.push_back(rightAfterGraspShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT,armData,executionTime);
    ros::Duration(0.3).sleep();

    chest_controller_->controlChest(0,0,0);
    ros::Duration(0.3).sleep();
    return true;

    //***************** done

}

bool CableTask::rotate_cable3(const geometry_msgs::Pose &goal, float executionTime)
{
    ToughControlCommon control_util(nh_);

    // setting initial poses
    ROS_INFO("grasp_cable: Initial Pose");
    std::vector< std::vector<float> > armData;
    armData.push_back(leftShoulderSeedInitial_);
    armTraj_.moveArmJoints(LEFT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    armData.clear();
    armData.push_back(rightShoulderSeedInitial_);
    armData.push_back(rightShoulderSeed_);
    armTraj_.moveArmJoints(RIGHT, armData, executionTime);
    ros::Duration(executionTime).sleep();

    // Opening grippers
    ROS_INFO("grasp_cable: Setting gripper position to open thumb");
    ROS_INFO("opening grippers");
    std::vector<double> gripper0,gripper1;
    gripper0={1.35, 1.35, 0.3, 0.0 ,0.0 };
    gripper1={1.2, 0.4, 0.35, 0.3 ,0.3 };

    gripper_.controlGripper(RIGHT,gripper0);
    ros::Duration(0.1).sleep();
    gripper_.controlGripper(RIGHT, gripper1);
    ros::Duration(0.2).sleep();

    geometry_msgs::Pose rightOffset;
    current_state_->getCurrentPose("/rightMiddleFingerPitch1Link",rightOffset,"/rightThumbRollLink");
    geometry_msgs::Pose intermGoal;

    intermGoal= goal;
    current_state_->transformPose(intermGoal,intermGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    intermGoal.orientation=rightHandOrientationAngle_.quaternion;
    current_state_->transformPose(intermGoal,intermGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);


    // setting a sead position without movement of chest
    intermGoal.position.z+=0.07;
    armTraj_.moveArmInTaskSpace(RIGHT, intermGoal, executionTime);
    ros::Duration(executionTime*2).sleep();
    control_util.stopAllTrajectories();

    geometry_msgs::Pose finalGoal;
    finalGoal = goal;
    current_state_->transformPose(finalGoal,finalGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    finalGoal.orientation=rightHandOrientationTop_.quaternion;
    current_state_->transformPose(finalGoal,finalGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);
    finalGoal.position.z+=0.03;
    // change in orientation
    current_state_->transformPose(finalGoal,finalGoal, VAL_COMMON_NAMES::WORLD_TF,rd_->getPelvisFrame());
    finalGoal.orientation= rightHandOrientationTop_.quaternion;
    current_state_->transformPose(finalGoal,finalGoal,rd_->getPelvisFrame(), VAL_COMMON_NAMES::WORLD_TF);

    std::vector<geometry_msgs::Pose> waypoints;

    waypoints.push_back(intermGoal);


    moveit_msgs::RobotTrajectory traj;


    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
        return false;
    }


    ROS_INFO("CableTask::grasp_choke: Calculated Traj");
    wholebody_controller_->executeTrajectory(RIGHT, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();

    std::vector<double> closeGrip={1.2, 0.4, 0.4, 0.4 ,0.4 };
    taskCommonUtils::slowGrip(nh_,RIGHT,gripper1,closeGrip);
    ros::Duration(0.3).sleep();
    gripper_.closeGripper(RIGHT);
    ros::Duration(2).sleep();

    //     Setting arm position to go leave cable and go up

    waypoints.clear();
    waypoints.push_back(finalGoal);



    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::grasp_choke: Trajectory is not planned 100% - retrying");
        return false;
    }


    ROS_INFO("CableTask::grasp_choke: Calculated Traj");
    wholebody_controller_->executeTrajectory(RIGHT, traj.joint_trajectory);
    ros::Duration(executionTime*2).sleep();


    //     Setting arm position to go leave cable and go up

    ROS_INFO("grasp_cable: Setting arm position to go up");
    armData.clear();
    armData.push_back(rightAfterRotateSeed_);
    armTraj_.moveArmJoints(RIGHT,armData,executionTime);
    ros::Duration(2).sleep();

    gripper_.openGripper(RIGHT);
    ros::Duration(0.3).sleep();

    chest_controller_->controlChest(0,0,0);
    ros::Duration(0.3).sleep();

    return true;

}

bool CableTask::drop_cable(RobotSide side)
{
    std::vector< std::vector<float> > armData;
    armData.clear();
    armData.push_back(rightCablePlaceSeed_);
    armTraj_.moveArmJoints(side,armData,2.0f);
    ros::Duration(2).sleep();

    gripper_.openGripper(side);
    ros::Duration(0.3).sleep();
    return true;
}

bool CableTask::allign_socket_axis(const geometry_msgs::Point &goal, float offset, float executionTime)
{
    std::vector<geometry_msgs::Pose> waypoints;
    geometry_msgs::Pose waypoint;
    current_state_->transformQuaternion(quat, quat);

    waypoint.position.x = goal.x;
    waypoint.position.y = goal.y;
    waypoint.position.z = goal.z+0.15; // 15 cm offset
    waypoint.orientation = quat.quaternion;
    waypoints.push_back(waypoint);

    waypoint.position.x = goal.x;
    waypoint.position.y = goal.y;
    waypoint.position.z = goal.z+offset; // manual offset
    waypoint.orientation = quat.quaternion;
    waypoints.push_back(waypoint);



    moveit_msgs::RobotTrajectory traj;

    // Sending waypoints to the planner
    std::vector< std::vector<float> > armData;
    if (right_arm_planner_choke->getTrajFromCartPoints(waypoints, traj, false)< 0.98){
        ROS_INFO("CableTask::allign socket: Trajectory is not planned 100% - retrying");
        ROS_INFO("grasp_cable: Setting arm position to dock cable");
        armData.clear();
        armData.push_back(rightAfterGraspShoulderSeed_);
        armTraj_.moveArmJoints(RIGHT,armData,executionTime);
        ros::Duration(0.3).sleep();
        return false;
    }

    wholebody_controller_->executeTrajectory(RIGHT,traj.joint_trajectory);

    ros::Duration(1).sleep();
}



