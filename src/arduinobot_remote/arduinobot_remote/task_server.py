#!/usr/bin/env python3
import rclpy                              # ROS2 Python 클라이언트 라이브러리 / ROS2 Python client library
from rclpy.node import Node               # ROS2 노드 기본 클래스 / Base class for ROS2 nodes
from rclpy.action import ActionServer     # 액션 서버 클래스 / Action server class
from arduinobot_msgs.action import ArduinobotTask  # ArduinobotTask 액션 인터페이스 / ArduinobotTask action interface
import numpy as np                             # 지연(sleep) 처리를 위한 표준 라이브러리 / Standard library for sleep
from moveit.planning import MoveItPy
from moveit.core.robot_state import RobotState

class TaskSever(Node):
    def __init__(self):
        # 노드 이름을 'task_server'로 초기화
        # Initialize node with name 'task_server'
        super().__init__('task_server')

        # 액션 서버 생성
        # - self          : 현재 노드 / current node
        # - Fibonacci     : 사용할 인터페이스 / interface to use
        # - 'fibonacci'   : 액션 서버 이름 (클라이언트가 이 이름으로 접속) / server name (clients connect by this name)
        # - self.goalCallback: Goal 수신 시 실행할 콜백 함수 / callback called when a goal is received
        self.action_server = ActionServer(
            self,
            ArduinobotTask,
            'task_server',
            self.goalCallback
        )
        self.arduinobot = MoveItPy(node_name="moveit_py")
        self.arduinobot_arm = self.arduinobot.get_planning_component('arm')
        self.arduinobot_gripper = self.arduinobot.get_planning_component('gripper')

    def goalCallback(self, goal_handle):
        # 클라이언트로부터 Goal 수신 시 자동 실행
        # Automatically called when a goal is received from the client

        # 수신한 Goal의 order 값을 로그로 출력
        # Log the received order value from the goal
        self.get_logger().info(
            'Received goal request with task_number %d' % goal_handle.request.task_number
        )

        arm_state = RobotState(self.arduinobot.get_robot_model())
        gripper_state = RobotState(self.arduinobot.get_robot_model())

        arm_joint_goal = []
        gripper_joint_goal = []

        if goal_handle.request.task_number == 0:
            arm_joint_goal = np.array([0.0, 0.0, 0.0])
            gripper_joint_goal = np.array([-0.7, 0.7])
        elif goal_handle.request.task_number == 1:
            arm_joint_goal = np.array([-1.14, -0.6, -0.07])
            gripper_joint_goal = np.array([0.0, 0.0])
        elif goal_handle.request.task_number == 2:
            arm_joint_goal = np.array([-1.57, 0.0, -0.9])
            gripper_joint_goal = np.array([0.0, 0.0])
        else:
            self.get_logger().error('Invalid Task Number')
            return
        
        arm_state.set_joint_group_positions("arm", arm_joint_goal)
        gripper_state.set_joint_group_positions("gripper", gripper_joint_goal)

        self.arduinobot_arm.set_start_state_to_current_state()
        self.arduinobot_gripper.set_start_state_to_current_state()

        self.arduinobot_arm.set_goal_state(robot_state=arm_state)
        self.arduinobot_gripper.set_goal_state(robot_state=gripper_state)

        arm_plan_result = self.arduinobot_arm.plan()
        gripper_plan_result = self.arduinobot_gripper.plan()

        if arm_plan_result and gripper_plan_result :
            self.arduinobot_arm.execute(arm_plan_result.trajectory, controllers=[])
            self.arduinobot_gripper.execute(gripper_plan_result.trajectory, controllers=[])
        else:
            self.get_logger().info('One or more planners failed')

        goal_handle.succeed()
        result = ArduinobotTask.Result()
        result.success = True
        return result

def main():
    # ROS2 초기화 / Initialize ROS2
    rclpy.init()

    # TaskSever 노드 인스턴스 생성
    # Create TaskSever node instance
    task_server = TaskSever()

    # 노드를 계속 실행 상태로 유지 (Goal 수신 대기)
    # Keep node running — waiting for goals from clients
    rclpy.spin(task_server)

    # 노드 종료 처리 / Destroy node on shutdown
    task_server.destroy_node()

    # ROS2 종료 / Shutdown ROS2
    rclpy.shutdown()

if __name__ == '__main__':
    main()
