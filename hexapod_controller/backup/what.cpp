void Control::publishOdometry(const geometry_msgs::Twist &gait_vel)
{
    current_time_odometry_ = ros::Time::now();
    double dt = (current_time_odometry_ - last_time_odometry_).toSec();
    double vth = gait_vel.angular.z * kali_A;
    double delta_th = vth * dt;
    pose_th_ += delta_th;
    odomNew.pose.pose.orientation.z = delta_th + odomOld.pose.pose.orientation.z;

    double vx = gait_vel.linear.x * kali_L;
    double vy = gait_vel.linear.y * kali_L;
    double delta_x = vx * dt;
    double delta_y = vy * dt;
    pose_x_ += delta_x;
    pose_y_ += delta_y;

    //calculate
    odomNew.pose.pose.position.x = odomOld.pose.pose.position.x + delta_x;
    odomNew.pose.pose.position.y = odomOld.pose.pose.position.y + delta_y;
    odomNew.pose.pose.orientation.z = delta_th + odomOld.pose.pose.orientation.z;

    // Uncomment the following lines if you need to debug the pose values
    // msg1.data = pose_x_;
    // msg2.data = pose_y_;
    // msg3.data = pose_th_;
    // chatter_pub1.publish(msg1);
    // chatter_pub2.publish(msg2);
    // chatter_pub3.publish(msg3);

    //Prevent lockup from a single bad cycle
    if (isnan(odomNew.pose.pose.position.x) || isnan(odomNew.pose.pose.position.y) || isnan(odomNew.pose.pose.position.z))
    {
        odomNew.pose.pose.position.x = odomOld.pose.pose.position.x;
        odomNew.pose.pose.position.y = odomOld.pose.pose.position.y;
        odomNew.pose.pose.orientation.z = odomOld.pose.pose.orientation.z;
    }

    // Save the pose data for the next cycle
    odomOld.pose.pose.position.x = odomNew.pose.pose.position.x;
    odomOld.pose.pose.position.y = odomNew.pose.pose.position.y;
    odomOld.pose.pose.orientation.z = odomNew.pose.pose.orientation.z;
    odomOld.header.stamp = odomNew.header.stamp;

    // Convert the orientation to a quaternion
    tf2::Quaternion q;
    q.setRPY(0, 0, odomNew.pose.pose.orientation.z);
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(odomNew.pose.pose.orientation.z);

    // First, we'll publish the transform over tf
    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = current_time_odometry_;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";
    odom_trans.transform.translation.x = odomNew.pose.pose.position.x;
    odom_trans.transform.translation.y = odomNew.pose.pose.position.y;
    odom_trans.transform.translation.z = body_.position.z;
    odom_trans.transform.rotation = odom_quat;

    // Uncomment the following line if you want to send the transform
    // odom_broadcaster.sendTransform(odom_trans);

    // Next, we'll publish the odometry message over ROS
    nav_msgs::Odometry odom;
    odom.header.stamp = current_time_odometry_;
    odom.header.frame_id = "odom";
    odom.child_frame_id = "base_link";

    // Set the position
    odom.pose.pose.position.x = odomNew.pose.pose.position.x;
    odom.pose.pose.position.y = odomNew.pose.pose.position.y;
    odom.pose.pose.position.z = body_.position.z;

    odom.pose.pose.orientation.x = q.x();
    odom.pose.pose.orientation.y = q.y();
    odom.pose.pose.orientation.z = odomNew.pose.pose.orientation.z;
    odom.pose.pose.orientation.w = q.w();

    // Set the covariance (if needed)
    odom.pose.covariance[0] = 0.00001;          // x
    odom.pose.covariance[7] = 0.00001;          // y
    odom.pose.covariance[14] = 0.00001;         // z
    odom.pose.covariance[21] = 1000000000000.0; // rot x
    odom.pose.covariance[28] = 1000000000000.0; // rot y
    odom.pose.covariance[35] = 0.001;           // rot z

    // Set the velocity
    odom.twist.twist.linear.x = vx;
    odom.twist.twist.linear.y = vy;
    odom.twist.twist.angular.z = vth;
    odom.twist.covariance = odom.pose.covariance; // If velocity covariance is the same as the position covariance

    odom_pub_.publish(odom);
    last_time_odometry_ = current_time_odometry_;
}
