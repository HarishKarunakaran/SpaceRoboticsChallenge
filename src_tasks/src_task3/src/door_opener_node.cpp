#include "src_task3/door_opener.h"

int main(int argc, char** argv){


    /** This code should accept the centre of valve from perception
     *  It's hard coded presently
     */

    ros::init(argc, argv, "door_opener_node");
    ros::NodeHandle nh;
    DoorOpener doorOpen(nh);
    geometry_msgs::Pose valveCenter;

    valveCenter.position.x = std::atof(argv[1]);
    valveCenter.position.y = std::atof(argv[2]);
    valveCenter.position.z = std::atof(argv[3]);
    valveCenter.orientation.w = 1;
    ROS_INFO("Starting door open task");

    doorOpen.openDoor(valveCenter);
    ros::spinOnce(); // but why?
    ros::Duration(1).sleep();

    return 0;
}
