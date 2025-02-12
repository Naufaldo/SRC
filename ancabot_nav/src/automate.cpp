#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Bool.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>

using namespace std;

// Action specification for move_base
typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

int main(int argc, char** argv) {
  // Initialize ROS
  ros::init(argc, argv, "simple_navigation_goals");

  // Create a ROS node handle
  ros::NodeHandle nh;

  // Create a publisher to publish the initial pose
  ros::Publisher initialPosePub = nh.advertise<geometry_msgs::PoseWithCovarianceStamped>("initialpose", 1);
  ros::Publisher headPub = nh.advertise<geometry_msgs::Twist>("/head_Tws", 1);
  ros::Publisher legHeightPub = nh.advertise<std_msgs::Bool>("/leg", 1);

  // Create the initial pose message
  geometry_msgs::PoseWithCovarianceStamped initialPoseMsg;
  initialPoseMsg.header.frame_id = "map";
  initialPoseMsg.pose.pose.position.x = 0.0;
  initialPoseMsg.pose.pose.position.y = 0.0;
  initialPoseMsg.pose.pose.position.z = 0.0;
  initialPoseMsg.pose.pose.orientation.x = 0.0;
  initialPoseMsg.pose.pose.orientation.y = 0.0;
  initialPoseMsg.pose.pose.orientation.z = 0.0;
  // initialPoseMsg.pose.pose.orientation.z = 1.5;
  // initialPoseMsg.pose.pose.orientation.w = 1;

  // Publish the initial pose
  initialPosePub.publish(initialPoseMsg);

  // Connect to the move_base action server
  MoveBaseClient ac("move_base", true);

  // Wait for the action server to come up
  while (!ac.waitForServer(ros::Duration(5.0))) {
    ROS_INFO("Waiting for the move_base action server to come up");
  }

  // Define the sequence of destinations
  double destinations[][6] = {

    // {x, y, w, Lifter, Gripper, leg_height}
    {0.46,0.0, 0.0, -2, 0, 0}, // Arah Korban 1
    // {0.46, 0.0,1.5, -2, 0, 0}, // Arah Korban 1
    // {0.46,0.1,1.5, 0, -1, 1},    // Ambil Korban 1
    // {0.46,0.0,0.0, -2, 0, 1},    // Siap Pergi ke Target 2
    {2.4,0.0,1, -2, 0, 1},    // Pergi ke Target 2
    // {2.4,0.0,-0.6, 0, -1, 1},    // Lepas korban di Target 2
    {2.4,0.0,1.5, -2, 0, 1},    // Jalan ke Target 3
    {2.4,0.7,1.5, -2, 0, 1},    // Jalan ke Target 3
    // {2.4658145904541016, 0.19893264770507812, 0.9196588897926539, -2, 0, 1},   // Simpen Korban 1
    // {2.1826889514923096, 0.6185263991355896, 0.03423934984405933, -2, 0, 1},    // Arah Korban 2
    // // Add more destinations in the sequence
    // {2.4959347248077393, 0.8430209755897522, 0.7359457450143778, -2, 0, 1},     // Simpen korban kelereng
    // {1.3070660829544067, 0.6406080722808838, 0.01420421912936522, -2, 0, 1}     // Arah Tangga
  };
  int numDestinations = sizeof(destinations) / sizeof(destinations[0]);

  for (int i = 0; i < numDestinations; i++) {
    // Create a new goal to send to move_base
    move_base_msgs::MoveBaseGoal goal;

    // Set the goal position and orientation based on the current destination
    goal.target_pose.header.frame_id = "map";
    goal.target_pose.header.stamp = ros::Time::now();
    goal.target_pose.pose.position.x = destinations[i][0];
    goal.target_pose.pose.position.y = destinations[i][1];
    goal.target_pose.pose.orientation.z = destinations[i][2];

    // Create and set the head and leg height messages
    geometry_msgs::Twist headTws;
    headTws.linear.x = destinations[i][3] * 0.625;
    headTws.linear.y = destinations[i][4] * 1.1;
    std_msgs::Bool legHeight;
    legHeight.data = destinations[i][5];

    ROS_INFO("Sending goal");
    ac.sendGoal(goal);

    // Publish the head and leg height commands
    headPub.publish(headTws);
    legHeightPub.publish(legHeight);

    // Wait until the robot reaches the goal
    ac.waitForResult();

    if (ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED)
      ROS_INFO("The robot has arrived at the goal location");
    else
      ROS_INFO("The robot failed to reach the goal location for some reason");
  }

  cout << "\nAll destinations visited. Exiting..." << endl;

  return 0;
}
