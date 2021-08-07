
// ROS Hexapod Teleop Joystick Node
// Copyright (c) 2016, Kevin M. Ochs
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the Kevin Ochs nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL KEVIN OCHS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: Kevin M. Ochs

#include <hexapod_teleop_joystick.h>

//==============================================================================
// Constructor
//==============================================================================

HexapodTeleopJoystick::HexapodTeleopJoystick(void)
{
    state_.data = true;
    imu_override_.data = true;
    NON_TELEOP = false; // Static but here for a safety precaution
    ros::param::get("STANDUP_BUTTON", STANDUP_BUTTON);
    ros::param::get("SITDOWN_BUTTON", SITDOWN_BUTTON);
    ros::param::get("BODY_ROTATION_BUTTON", BODY_ROTATION_BUTTON);
    ros::param::get("FORWARD_BACKWARD_AXES", FORWARD_BACKWARD_AXES);
    ros::param::get("LEFT_RIGHT_AXES", LEFT_RIGHT_AXES);
    ros::param::get("YAW_ROTATION_AXES", YAW_ROTATION_AXES);
    ros::param::get("PITCH_ROTATION_AXES", PITCH_ROTATION_AXES);
    ros::param::get("MAX_METERS_PER_SEC", MAX_METERS_PER_SEC);
    ros::param::get("MAX_RADIANS_PER_SEC", MAX_RADIANS_PER_SEC);
    ros::param::get("NON_TELEOP", NON_TELEOP);
    joy_sub_ = nh_.subscribe<sensor_msgs::Joy>("/joy", 5, &HexapodTeleopJoystick::joyCallback, this);
    body_scalar_pub_ = nh_.advertise<geometry_msgs::AccelStamped>("/body_scalar", 100);
    head_scalar_pub_ = nh_.advertise<geometry_msgs::AccelStamped>("/head_scalar", 100);
    cmd_vel_pub_ = nh_.advertise<geometry_msgs::Twist>("cmd_vel", 100);
    state_pub_ = nh_.advertise<std_msgs::Bool>("/state", 100);
    imu_override_pub_ = nh_.advertise<std_msgs::Bool>("/imu/imu_override", 100);
    key = getch();

    if (key == 'a')
    {
        if (state_.data == false)
        {
            state_.data = true;
        }
    }

    else if (key == 's')
    {
        if (state_.data == true)
        {
            state_.data = false;
        }
    }
    // If the key corresponds to a key in moveBindings
    else if (moveBindings.count(key) == 1)
    {
        // Grab the direction data
        x = moveBindings[key][0];
        y = moveBindings[key][1];
        z = moveBindings[key][2];
        th = moveBindings[key][3];

        printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
    }

    // Otherwise if it corresponds to a key in speedBindings
    else if (speedBindings.count(key) == 1)
    {
        // Grab the speed data
        speed = speed * speedBindings[key][0];
        turn = turn * speedBindings[key][1];

        printf("\rCurrent: speed %f\tturn %f | Last command: %c   ", speed, turn, key);
    }

    // Otherwise, set the robot to stop
    else
    {
        x = 0;
        y = 0;
        z = 0;
        th = 0;

        // If ctrl-C (^C) was pressed, terminate the program
        if (key == '\x03')
        {
            printf("\n\n                 .     .\n              .  |\\-^-/|  .    \n             /| } O.=.O { |\\\n\n                 CH3EERS\n\n");
            break;
        }

        printf("\rCurrent: speed %f\tturn %f | Invalid command! %c", speed, turn, key);
    }

    // Update the Twist message
    cmd_vel_pub_.linear.x = x * speed;
    cmd_vel_pub_.linear.y = y * speed;
    cmd_vel_pub_.linear.z = z * speed;

    cmd_vel_pub_.angular.x = 0;
    cmd_vel_pub_.angular.y = 0;
    cmd_vel_pub_.angular.z = th * turn;
}

//==============================================================================
// Joystick call reading joystick topics
//==============================================================================

void HexapodTeleopJoystick::joyCallback(const sensor_msgs::Joy::ConstPtr &joy)
{
    ros::Time current_time = ros::Time::now();
    if (joy->buttons[STANDUP_BUTTON] == 1)
    {
        if (state_.data == false)
        {
            state_.data = true;
        }
    }

    if (joy->buttons[SITDOWN_BUTTON] == 1)
    {
        if (state_.data == true)
        {
            state_.data = false;
        }
    }

    // Body rotation L1 Button for testing
    if (joy->buttons[BODY_ROTATION_BUTTON] == 1)
    {
        imu_override_.data = true;
        body_scalar_.header.stamp = current_time;
        body_scalar_.accel.angular.x = -joy->axes[LEFT_RIGHT_AXES];
        body_scalar_.accel.angular.y = -joy->axes[FORWARD_BACKWARD_AXES];
        head_scalar_.header.stamp = current_time;
        head_scalar_.accel.angular.z = joy->axes[YAW_ROTATION_AXES];
        head_scalar_.accel.angular.y = joy->axes[PITCH_ROTATION_AXES];
    }
    else
    {
        imu_override_.data = false;
    }

    // Travelling
    if (joy->buttons[BODY_ROTATION_BUTTON] != 1)
    {
        cmd_vel_.linear.x = joy->axes[FORWARD_BACKWARD_AXES] * MAX_METERS_PER_SEC;
        cmd_vel_.linear.y = -joy->axes[LEFT_RIGHT_AXES] * MAX_METERS_PER_SEC;
        cmd_vel_.angular.z = joy->axes[YAW_ROTATION_AXES] * MAX_RADIANS_PER_SEC;
    }
}

/////////////////////////////////////////////////////////
int getch(void)
{
    int ch;
    struct termios oldt;
    struct termios newt;

    // Store old settings, and copy to new settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // Make required changes and apply the settings
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_iflag |= IGNBRK;
    newt.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF);
    newt.c_lflag &= ~(ICANON | ECHO | ECHOK | ECHOE | ECHONL | ISIG | IEXTEN);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &newt);

    // Get the current character
    ch = getchar();

    // Reapply old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}
////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    ros::init(argc, argv, "hexapod_teleop_joystick");
    HexapodTeleopJoystick hexapodTeleopJoystick;

    ros::AsyncSpinner spinner(1); // Using 1 threads
    spinner.start();

    ros::Rate loop_rate(100); // 100 hz
    while (ros::ok())
    {
        if (hexapodTeleopJoystick.NON_TELEOP == false) // If True, assumes you are sending these from other packages
        {
            hexapodTeleopJoystick.cmd_vel_pub_.publish(hexapodTeleopJoystick.cmd_vel_);
            hexapodTeleopJoystick.body_scalar_pub_.publish(hexapodTeleopJoystick.body_scalar_);
            hexapodTeleopJoystick.head_scalar_pub_.publish(hexapodTeleopJoystick.head_scalar_);
        }
        hexapodTeleopJoystick.state_pub_.publish(hexapodTeleopJoystick.state_); // Always publish for means of an emergency shutdown type situation
        hexapodTeleopJoystick.imu_override_pub_.publish(hexapodTeleopJoystick.imu_override_);
        loop_rate.sleep();
    }
}
