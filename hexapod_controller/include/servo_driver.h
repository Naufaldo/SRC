#ifndef SERVO_DRIVER_H_
#define SERVO_DRIVER_H_

#include <cmath>
#include <ros/ros.h>
#include <dynamixel_sdk/dynamixel_sdk.h>
#include <sensor_msgs/JointState.h>

// Default setting
#define BAUDRATE 115200
#define DEVICENAME "/dev/Dynamixel" // Check which port is being used on your controller
#define PROTOCOL_VERSION 1.0      // See which protocol version is used in the Dynamixel
#define TORQUE_ON 1
#define TORQUE_OFF 0
#define LEN_GOAL_POSITION 2
//==============================================================================
// Define the class(s) for Servo Drivers.
//==============================================================================

class ServoDriver
{
public:
    ServoDriver(void);
    ~ServoDriver(void);
    void transmitServoPositions(const sensor_msgs::JointState &joint_state);
    void makeSureServosAreOn(const sensor_msgs::JointState &joint_state);
    void freeServos(void);

private:
    dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);               // Initialize PacketHandler instance
    dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION); // Set the protocol version
    uint8_t dxl_error = 0;                                                                                  // Dynamixel error
    uint16_t dxl_present_position = 0;                                                                      // Present position
    uint16_t currentPos;
    uint8_t param_goal_position[2];
    void convertAngles(const sensor_msgs::JointState &joint_state);
    std::vector<int> cur_pos_;                   // Current position of servos
    std::vector<int> goal_pos_;                  // Goal position of servos
    std::vector<int> pose_steps_;                // Increment to use going from current position to goal position
    std::vector<int> write_pos_;                 // Position of each servo for sync_write packet
    std::vector<double> OFFSET;                  // Physical hardware offset of servo horn
    std::vector<int> ID;                         // Servo IDs
    std::vector<int> TICKS;                      // Total number of ticks, meaning resolution of dynamixel servo
    std::vector<int> CENTER;                     // Center value of dynamixel servo
    std::vector<double> RAD_TO_SERVO_RESOLUTION; // Radians to servo conversion
    std::vector<double> MAX_RADIANS;             // Max rotation your servo is manufactured to do. i.e. 360 degrees for MX etc.
    XmlRpc::XmlRpcValue SERVOS;                  // Servo map from yaml config file
    std::vector<int> servo_orientation_;         // If the servo is physically mounted backwards this sign is flipped
    std::vector<std::string> servo_map_key_;
    bool portOpenSuccess = false;
    bool torque_on = true;
    bool torque_off = true;
    bool writeParamSuccess = true;
    bool servos_free_;
    int SERVO_COUNT;
    int TORQUE_ENABLE, PRESENT_POSITION_L, GOAL_POSITION_L, INTERPOLATION_LOOP_RATE;
};

#endif // SERVO_DRIVER_H_
