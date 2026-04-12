from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():

    # 인자 선언
    cam_x     = DeclareLaunchArgument('cam_x',     default_value='0')
    cam_y     = DeclareLaunchArgument('cam_y',     default_value='0.45')
    cam_z     = DeclareLaunchArgument('cam_z',     default_value='0.2')
    cam_roll  = DeclareLaunchArgument('cam_roll',  default_value='0')
    cam_pitch = DeclareLaunchArgument('cam_pitch', default_value='-0.5')
    cam_yaw   = DeclareLaunchArgument('cam_yaw',   default_value='1.57')

    model_arg = DeclareLaunchArgument(
        name="model",
        default_value=os.path.join(get_package_share_directory("arduinobot_description"), "urdf", "arduinobot.urdf.xacro"),
        description="Absolute path to the robot URDF file"
    )

    # xacro 명령에 인자 추가
    robot_description = ParameterValue(Command([
        "xacro ", LaunchConfiguration("model"),
        " cam_x:=",     LaunchConfiguration("cam_x"),
        " cam_y:=",     LaunchConfiguration("cam_y"),
        " cam_z:=",     LaunchConfiguration("cam_z"),
        " cam_roll:=",  LaunchConfiguration("cam_roll"),
        " cam_pitch:=", LaunchConfiguration("cam_pitch"),
        " cam_yaw:=",   LaunchConfiguration("cam_yaw"),
    ]))

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": robot_description}]
    )

    joint_state_publisher_gui = Node(
        package="joint_state_publisher_gui",
        executable="joint_state_publisher_gui",
    )

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", os.path.join(get_package_share_directory("arduinobot_description"), "rviz", "display.rviz")]
    )

    return LaunchDescription([
        cam_x, cam_y, cam_z, cam_roll, cam_pitch, cam_yaw,  # ← 추가
        model_arg,
        robot_state_publisher,
        joint_state_publisher_gui,
        rviz_node,
    ])

