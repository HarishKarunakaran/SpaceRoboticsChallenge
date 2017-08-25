#ifndef ROBOT_WALKER_H
#define ROBOT_WALKER_H

#include "ros/ros.h"
#include"geometry_msgs/Pose2D.h"
#include <humanoid_nav_msgs/PlanFootsteps.h>
#include "ihmc_msgs/FootstepDataListRosMessage.h"
#include "ihmc_msgs/FootstepDataRosMessage.h"
#include "ihmc_msgs/FootstepStatusRosMessage.h"
#include "ihmc_msgs/FootTrajectoryRosMessage.h"
#include "ihmc_msgs/WholeBodyTrajectoryRosMessage.h"
#include<ihmc_msgs/EndEffectorLoadBearingRosMessage.h>
#include <geometry_msgs/TransformStamped.h>
#include "std_msgs/String.h"
#include "ros/time.h"
#include "tf/tf.h"
#include <tf/transform_listener.h>
#include <val_common/val_common_defines.h>
#include <val_controllers/robot_state.h>



/**
 * @brief The RobotWalker class is an API over IHMC footstep messages.
 * It uses a footstep planner to plan footsteps.
 * It can plan footstep trajectories.
 */
class RobotWalker
{

public:
    RobotStateInformer *current_state_;
    static int id ;

    /**
     * @brief RobotWalker provides access to the footsteps of robot. It can be used
     * to get current position of the steps or to make  the robot walk given number of steps.
     * @param nh                Ros Nodehandle to which the publishers and subscribers will be attached
     * @param inTransferTime    Transfer time in seconds for every step
     * @param inSwingTime       Swing time in seconds for every step
     * @param inMode            Execution mode of the steps. this can be 0 = OVERRIDE or 1 = QUEUE.
     * @param swingHeight       Swing height to be used for every step
     * @todo Implement singleton pattern. There should be only one object of this class available of this class.
     */
    RobotWalker(ros::NodeHandle nh, double inTransferTime = 1.5,double inSwingTime =1.5 , int inMode = 0, double swing_height_ = 0.2);
    ~RobotWalker();

    /**
     * @brief walkToGoal walks to a given 2D point in a map. It needs a map either from map server or from octomap server
     * @param goal      Pose2d message giving position and orientation of goal point.
     * @return true     If footstep planning is successful else false
     */
    bool walkToGoal(geometry_msgs::Pose2D &goal, bool waitForSteps=true);

    /**
     * @brief walkNSteps Makes the robot walk given number of steps.
     * @param numSteps   Number of steps to walk
     * @param xOffset    Distance to travel forward in half stride. First step is half the stride length as both the feet are assumed to be together.
     *                   This is defined wrt World.
     * @param yOffset    Distance to travel sideways in half a stride length. First step is half the stride length as both the feet are assumed to be together.
     *                   This is defined wrt World.
     * @param continous  If this is set to true, the robot stops with one foot forward. if it is false, both the feet are together at the end of walk.
     * @param startLeg   Leg to be used to start walking. It can be RIGHT or LEFT
     * @return
     */
    bool walkNSteps(int numSteps, float xOffset, float yOffset=0.0f, bool continous=false, armSide startLeg=RIGHT, bool waitForSteps=true);

    /**
     * @brief walkNSteps Makes the robot walk given number of steps.
     * @param numSteps   Number of steps to walk
     * @param xOffset    Distance to travel forward in half stride. First step is half the stride length as both the feet are assumed to be together.
     *                   This is defined wrt Pelvis. Forward - positive x axis , Left - positive y axis
     * @param yOffset    Distance to travel sideways in half a stride length. First step is half the stride length as both the feet are assumed to be together.
     *                   This is defined wrt Pelvis. Forward - positive x axis , Left - positive y axis
     * @param continous  If this is set to true, the robot stops with one foot forward. if it is false, both the feet are together at the end of walk.
     * @param startLeg   Leg to be used to start walking. It can be RIGHT or LEFT
     * @return
     */
    bool walkNStepsWRTPelvis(int numSteps, float xOffset, float yOffset=0.0f, bool continous=false, armSide startLeg=RIGHT, bool waitForSteps=true);

    /**
     * @brief walkPreComputedSteps If the steps to be sent to the robot are not identical, use this function to send steps that are precomputed.
     * @param xOffset  is a vector of float. Each value represents offset in x direction of individual step
     * @param yOffset  is a vector of float with size same as that of x_offset. Each value represents offset in y direction of individual step
     * @param startleg  Leg to be used to start walking. It can be RIGHT or LEFT
     * @return
     */
    bool walkPreComputedSteps(const std::vector<float> xOffset, const std::vector<float> yOffset, armSide startleg);

    /**
     * @brief walkGivenSteps This function publishes a given list of ros messages of type ihmc_msgs::FootstepDataListRosMessage to the robot.
     * @param list           List of steps in ihmc_msgs::FootstepDataListRosMessage format.
     * @return
     */
    bool walkGivenSteps(ihmc_msgs::FootstepDataListRosMessage& list , bool waitForSteps=true);

    /**
     * @brief setWalkParms      Set the values of walking parameters
     * @param InTransferTime    Transfer time is the time required for the robot to switch its weight from one to other while walking.
     * @param InSwingTime       Swing time is the time required for the robot to swing its leg to the given step size.
     * @param InMode            Execution mode defines if steps are to be queued with previous steps or override and start a walking message. Only Override is supported in this version.
     * @todo create separate messages for each of the parameters.
     */
    inline void setWalkParms(float inTransferTime,float inSwingTime, int inMode)
    {
        this->transfer_time_ = inTransferTime;
        this->swing_time_ = inSwingTime;
        this->execution_mode_ = inMode;
    }

    /**
     * @brief getSwingHeight fetch the swing height used for steps.
     * @return returns the swing_height of the current object.
     */
    double getSwingHeight() const;

    /**
     * @brief setSwingHeight Sets swing_height for walking.
     * @param value          Value is the swing_height that determines how high a feet should be lifted while walking in meters. It should be between 0.1 and 0.25     *
     */
    inline void setSwingHeight(double value)
    {
        swing_height_ = value;
    }

    /**
     * @brief turn contains precomputed footsteps to make a left or right 90 degree turn
     * @param side left- anticlockwise, right - clockwise.
     * DO NOT USE. ROBOT MIGHT FALL.
     * @return
     */
    bool turn(armSide side);

    /**
     * @brief walkLocalPreComputedSteps walks predefined steps which could have varying step length and step widths. This is defined wrt Pelvis frame.
     * @param xOffset  Is a vector of float. Each value represents offset in x direction of individual step.
     *                 you can define a set a predefined step length offsets.
     * @param yOffset  Is a vector of float with size same as that of x_offset. Each value represents offset in y direction of individual step
     *                 you can define a set a predefined step widths offsets.
     * @param startleg Leg to be used to start walking. It can be RIGHT or LEFT
     * @return
     */
    bool walkLocalPreComputedSteps(const std::vector<float> xOffset, const std::vector<float> yOffset, armSide startLeg);

    /**
     * @brief curlLeg would curl the leg behind with a defined radius. it is similar to the flamingo position.
     * @param side LEFT/RIGHT
     * @param radius is the radius of this backward curled trajectory
     * @return
     */
    bool curlLeg(armSide side, float radius);

    /**
     * @brief placeLeg is used when the swing leg is in a arbitrary lifted position and has to be placed within a z-offset with the support leg.
     *        it can be thought as a way to bring the robot from the flamingo position to a normal position with a z-offset.
     * @param side LEFT/RIGHT
     * @param offset is the offset in the z-axis
     * @return
     */
    bool placeLeg(armSide side, float offset=0.1f);

    /**
     * @brief nudgeFoot nudges the foot forward or backward
     * @param side LEFT/RIGHT
     * @param distance is the offset distance
     * @return
     */
    bool nudgeFoot(armSide side, float distance);

    /**
     * @brief getCurrentStep gives the current position of the robot wrt to world frame
     * @param side LEFT/ RIGHT leg
     * @param foot is the foot msg which stores the location of the foot in world frame.
     */
    void getCurrentStep(int side , ihmc_msgs::FootstepDataRosMessage& foot);

    /**
     * @brief raiseLeg raises the leg forward at a desired height.
     * @param side  LEFT/RIGHT
     * @param height is the height to raise the leg
     * @param stepLength is the forward step length
     * @return
     */
    bool raiseLeg(armSide side, float height,float stepLength);

    /**
     * @brief loadEEF loads the endeffector to distribute weight evenly in both legs.
     * @param side  LEFT/RIGHT
     * @param load enum stating loading or unloading.
     * Note: After current testing, it seems that only loading condition works. Means that if the robot is in flamingo position,
     * it can be bought back to normal stance position with weight evenly distributed in both legs.
     */
    void loadEEF(armSide side, EE_LOADING load);

    /**
     * @brief walkRotate rotates the robot by desired angle. this is relative to the current yaw angle.
     * @param angle is the angle expressed in radians
     * @return
     */
    bool walkRotate(float angle);

    /**
     * @brief climbStair is a function which can make the robot climb steps given a list of step placement locations.
     * @param xOffset is a list of forward displacements.
     * @param zOffset is a list of height displacements.
     * @param startLeg LEFT/ RIGHT
     * @return
     */
    bool climbStair(const std::vector<float> xOffset, const std::vector<float> zOffset, armSide startLeg);

    /**
     * @brief getFootstep plans footsteps to a goal.
     * @param goal is 2D goal pose
     * @param Is a list of footstep list
     * @return
     */
    bool getFootstep(geometry_msgs::Pose2D &goal,ihmc_msgs::FootstepDataListRosMessage &list);

private:

    double                      transfer_time_,swing_time_, swing_height_;
    int                         execution_mode_, step_counter_;
    ros::NodeHandle             nh_;
    ros::Time                   cbTime_;
    ros::Publisher              footsteps_pub_ ,nudgestep_pub_,loadeff_pub;
    ros::Subscriber             footstep_status_ ;
    ros::ServiceClient          footstep_client_ ;
    tf::TransformListener       tf_listener_;
    std_msgs::String            right_foot_frame_,left_foot_frame_;

    void footstepStatusCB(const ihmc_msgs::FootstepStatusRosMessage & msg);
    void waitForSteps( int numSteps);
    ihmc_msgs::FootstepDataRosMessage::Ptr getOffsetStep(int side, float x, float y);
    ihmc_msgs::FootstepDataRosMessage::Ptr getOffsetStepWRTPelvis(int side , float x, float y);

};

#endif  //ROBOT_WALKER_H