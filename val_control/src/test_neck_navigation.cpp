#include <val_control/val_neck_navigation.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  ros::init(argc, argv, "test_neck_navigation");
  ros::NodeHandle nh;
  ROS_INFO("Moving the neck");

  NeckTrajectory neckTraj(nh);

  neckTraj.moveNeckJoints({{ 1.57f, 1.57f, 0.0f }}, 2.0f);

  while(ros::ok())
  {}

  return 0;
}
