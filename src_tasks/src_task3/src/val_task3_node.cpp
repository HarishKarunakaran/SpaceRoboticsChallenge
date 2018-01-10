#include <src_task3/val_task3_node.h>

#define foreach BOOST_FOREACH

task3Node::task3Node(ros::NodeHandle nh): nh_(nh)
{
  task3_ = valTask3::getValTask3(nh);
}

task3Node::~task3Node()
{
    ROS_INFO("task3Node::Destrutor called");
    if (task3_ != nullptr) delete task3_;
}

void task3Node::registerStateMethods()
{
  // Register all the functions
  // register the api's for states
  LocalTasks::registrate("STATE_INIT",                         std::bind(&valTask3::initTask,             task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_STAIRS",                std::bind(&valTask3::detectStairsTask,     task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_STAIRS",               std::bind(&valTask3::walkToStairsTask,     task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_CLIMB_STEPS",                  std::bind(&valTask3::climbStepsTask,       task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_DOOR_HANDLE",           std::bind(&valTask3::detectDoorHandleTask, task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_OPEN_DOOR",                    std::bind(&valTask3::openDoorTask,         task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_THROUGH_DOOR",            std::bind(&valTask3::walkthroughDoorTask,  task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_LEAK_TOOL",             std::bind(&valTask3::detectLeakToolTask,   task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_LEAK_TOOL",            std::bind(&valTask3::walkToLeakToolTask,   task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_PICK_LEAK_TOOL",               std::bind(&valTask3::pickLeakTool,         task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_DETECT_LEAK_LOCATION", std::bind(&valTask3::walkToDetectLeak,     task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_LEAK_LOCATION",         std::bind(&valTask3::detectLeakTask,       task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_REPAIR_TOOL",           std::bind(&valTask3::detectRepairToolTask, task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_REPAIR_TOOL",          std::bind(&valTask3::walkToRepairToolTask, task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_PICK_REPAIR_TOOL",             std::bind(&valTask3::pickRepairTool,       task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_LEAK_LOCATION",        std::bind(&valTask3::walkToLeakTask,       task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_LEAK_REPAIR",                  std::bind(&valTask3::leakRepairTask,       task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_DETECT_FINISH",                std::bind(&valTask3::detectFinishTask,     task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_WALK_TO_FINISH",               std::bind(&valTask3::walkToFinishTask,     task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_END",                          std::bind(&valTask3::endTask,              task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  LocalTasks::registrate("STATE_ERROR",                        std::bind(&valTask3::errorTask,            task3_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void task3Node::paramUpdateCallback(src_task3::task3_parametersConfig &config, uint32_t level)
{
  ROS_INFO("update the goal, x: %f, y: %f, theta: %f",  config.groups.stairwalkpose.x_sw ,  config.groups.stairwalkpose.y_sw,  config.groups.stairwalkpose.theta_sw);

  geometry_msgs::Pose2D pose2d;
  geometry_msgs::Point  location;
  geometry_msgs::Pose pose;

  if (task3_ != nullptr)
  {
    //update the walk goal location to stair case
    pose2d.x = config.groups.stairwalkpose.x_sw;
    pose2d.y = config.groups.stairwalkpose.y_sw;
    pose2d.theta = config.groups.stairwalkpose.theta_sw;
    task3_->setStairDetectWalkPose(pose2d);

    // update the handle center
    pose.position.x = config.groups.handlecenter.x_hc;
    pose.position.y = config.groups.handlecenter.y_hc;
    pose.position.z = config.groups.handlecenter.z_hc;
    task3_->setHandleCenter(pose);

    // update table
    pose2d.x = config.groups.tablewalkpose.x_tw;
    pose2d.y = config.groups.tablewalkpose.y_tw;
    pose2d.theta = config.groups.tablewalkpose.theta_tw;
    task3_->setTableWalkPose(pose2d);

    // set leak detector location
    location.x = config.groups.leakdetectorloc.x_ld;
    location.y = config.groups.leakdetectorloc.y_ld;
    location.z = config.groups.leakdetectorloc.z_ld;
    task3_->setLeakDetectorLoc(location);

    // set leak wall walk pose
    pose2d.x = config.groups.leakwallpose.x_lw;
    pose2d.y = config.groups.leakwallpose.y_lw;
    pose2d.theta = config.groups.leakwallpose.theta_lw;
    task3_->setLeakWallPose(pose2d);

    // set leak location
    location.x = config.groups.leakloc.x_l;
    location.y = config.groups.leakloc.y_l;
    location.z = config.groups.leakloc.z_l;
    task3_->setLeakLoc(location);

    // set repair tool location
    location.x = config.groups.repairtoolloc.x_rt;
    location.y = config.groups.repairtoolloc.y_rt;
    location.z = config.groups.repairtoolloc.z_rt;
    task3_->setRepairToolLoc(location);

    // set finish box walk pose
    pose2d.x = config.groups.finishboxwalkpose.x_fb;
    pose2d.y = config.groups.finishboxwalkpose.y_fb;
    pose2d.theta = config.groups.finishboxwalkpose.theta_fb;
    task3_->setFinishBoxPose(pose2d);
  }
}

int main(int argc, char** argv){

  ros::init(argc, argv, "task3");
  ros_decision_making_init(argc, argv);
  ros::NodeHandle nh;
  RosEventQueue *q = new RosEventQueue();

  task3Node task3Node(nh);

  ROS_INFO("Preparing task3...");

  // register the state execution methods
  task3Node.registerStateMethods();

  dynamic_reconfigure::Server<src_task3::task3_parametersConfig> service;
  dynamic_reconfigure::Server<src_task3::task3_parametersConfig>::CallbackType callback_type;
  callback_type = std::bind(&task3Node::paramUpdateCallback, &task3Node, std::placeholders::_1, std::placeholders::_2);
  service.setCallback(callback_type);
  ros::AsyncSpinner spinner(1);
  spinner.start();

  ROS_INFO("Starting Task 3");
  Fsmsrc_task3(NULL, q, "src_task3");

  spinner.stop();

  return 0;

}
