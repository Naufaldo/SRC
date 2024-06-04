#include <ros/ros.h>
#include <control.h>
#include <gait.h>
#include <ik.h>
#include <geometry_msgs/PoseStamped.h>
#include <std_msgs/Float64.h>

// Function to publish joint commands
void publishJointCommands(const hexapod_msgs::LegsJoints &legs)
{
    static ros::NodeHandle nh;
    static std::vector<ros::Publisher> joint_publishers;

    if (joint_publishers.empty()) {
        std::vector<std::string> joint_names = {"LF", "LM", "LR", "RF", "RM", "RR"};
        for (const auto &joint_name : joint_names) {
            joint_publishers.push_back(nh.advertise<std_msgs::Float64>("/Golem/coxa_joint_" + joint_name + "_position_controller/command", 10));
            joint_publishers.push_back(nh.advertise<std_msgs::Float64>("/Golem/femur_joint_" + joint_name + "_position_controller/command", 10));
            joint_publishers.push_back(nh.advertise<std_msgs::Float64>("/Golem/tibia_joint_" + joint_name + "_position_controller/command", 10));
            joint_publishers.push_back(nh.advertise<std_msgs::Float64>("/Golem/tarsus_joint_" + joint_name + "_position_controller/command", 10));
        }
    }

    for (int leg_index = 0; leg_index < 6; ++leg_index) {
        std_msgs::Float64 msg;
        msg.data = legs.leg[leg_index].coxa;
        joint_publishers[16 * leg_index].publish(msg);

        msg.data = legs.leg[leg_index].femur;
        joint_publishers[16 * leg_index + 1].publish(msg);

        msg.data = legs.leg[leg_index].tibia;
        joint_publishers[16 * leg_index + 2].publish(msg);

        msg.data = legs.leg[leg_index].tarsus;
        joint_publishers[16 * leg_index + 3].publish(msg);
    }
}



//=============================================================================
// Main
//=============================================================================

int main(int argc, char **argv)
{
    ros::init(argc, argv, "hexapod_controller");

    // Create class objects
    Control control;
    Gait gait;
    Ik ik;

    // Establish initial leg positions for default pose in robot publisher
    gait.gaitCycle(control.cmd_vel_, &control.feet_, &control.gait_vel_);
    ik.calculateIK(control.feet_, control.body_, &control.legs_);
    control.publishJointStates(control.legs_, control.head_, &control.joint_state_);
    control.publishOdometry(control.gait_vel_);
    control.publishTwist(control.gait_vel_);

    ros::Time current_time_, last_time_;
    current_time_ = ros::Time::now();
    last_time_ = ros::Time::now();

    ros::AsyncSpinner spinner(2); // Using 2 threads
    spinner.start();
    ros::Rate loop_rate(300);
    while (ros::ok())
    {
        current_time_ = ros::Time::now();
        double dt = (current_time_ - last_time_).toSec();
        // Divide cmd_vel by the loop rate to get appropriate velocities for gait period
        control.partitionCmd_vel(&control.cmd_vel_);

        // Start button on controller has been pressed stand up
        if (control.getHexActiveState() == true && control.getPrevHexActiveState() == false)
        {
            ROS_INFO("Hexapod standing up.");
            while (control.body_.position.z < control.STANDING_BODY_HEIGHT)
            {
                control.body_.position.z = control.body_.position.z + 0.001; // 1 mm increment

                // IK solver for legs and body orientation
                ik.calculateIK(control.feet_, control.body_, &control.legs_);

                // Publish new positions
                publishJointCommands(control.legs_);

                control.publishJointStates(control.legs_, control.head_, &control.joint_state_);
                control.publishOdometry(control.gait_vel_);
                control.publishTwist(control.gait_vel_);
            }
            control.setPrevHexActiveState(true);
        }
        
        // We are live and standing up
        if (control.getHexActiveState() == true && control.getPrevHexActiveState() == true)
        {
            // Gait Sequencer
            gait.gaitCycle(control.cmd_vel_, &control.feet_, &control.gait_vel_);
            control.publishTwist(control.gait_vel_);

            // IK solver for legs and body orientation
            ik.calculateIK(control.feet_, control.body_, &control.legs_);

            // Publish new positions
            publishJointCommands(control.legs_);

            control.publishJointStates(control.legs_, control.head_, &control.joint_state_);
            control.publishOdometry(control.gait_vel_);
            control.publishTwist(control.gait_vel_);

            // Set previous hex state of last loop so we know if we are shutting down on the next loop
            control.setPrevHexActiveState(true);
        }

        // Shutting down hex so let us do a gradual sit down and turn off torque
        if (control.getHexActiveState() == false && control.getPrevHexActiveState() == true)
        {
            ROS_INFO("Hexapod sitting down.");
            while (control.body_.position.z > 0)
            {
                control.body_.position.z = control.body_.position.z - 0.001; // 1 mm increment

                // Gait Sequencer called to make sure we are on all six feet
                gait.gaitCycle(control.cmd_vel_, &control.feet_, &control.gait_vel_);

                // IK solver for legs and body orientation
                ik.calculateIK(control.feet_, control.body_, &control.legs_);

                // Publish new positions
                publishJointCommands(control.legs_);

                control.publishJointStates(control.legs_, control.head_, &control.joint_state_);
                control.publishOdometry(control.gait_vel_);
                control.publishTwist(control.gait_vel_);
            }

            // Locomotion is now shut off
            control.setPrevHexActiveState(false);
        }
        // Sitting down with servo torque off. Publish jointState message every half second
        if (control.getHexActiveState() == false && control.getPrevHexActiveState() == false)
        {
            control.publishJointStates(control.legs_, control.head_, &control.joint_state_);
            control.publishOdometry(control.gait_vel_);
            control.publishTwist(control.gait_vel_);
        }
        control.publishOdometry(control.gait_vel_);
        control.publishTwist(control.gait_vel_);

        loop_rate.sleep();
        last_time_ = current_time_;
    }
    return 0;
}
