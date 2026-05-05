# Task Server C++ 실습 / Task Server C++ Practice

---

## 📌 목차 / Table of Contents

1. [실습 개요 / Overview](#1-실습-개요--overview)
2. [액션 인터페이스 정의 / Action Interface](#2-액션-인터페이스-정의--action-interface)
3. [Task Server C++ 구현 / C++ Implementation](#3-task-server-c-구현--c-implementation)
4. [빌드 설정 / Build Configuration](#4-빌드-설정--build-configuration)
5. [런치 파일 / Launch File](#5-런치-파일--launch-file)
6. [실행 순서 / Execution Order](#6-실행-순서--execution-order)
7. [태스크 테스트 / Task Test](#7-태스크-테스트--task-test)

---

## 1. 실습 개요 / Overview

> **한국어**
> MoveIt2 C++ API와 ROS2 Action Server를 결합하여
> 태스크 ID를 수신하면 해당 로봇 동작을 실행하는 Task Server를 구현합니다.
> ROS2 Humble에서는 `moveit_py`가 지원되지 않으므로 C++로 구현해야 합니다.

> **English**
> Combines MoveIt2 C++ API with ROS2 Action Server to implement a Task Server
> that executes predefined robot motions based on received task IDs.
> Since `moveit_py` is not supported on ROS2 Humble, C++ implementation is required.

> ⚠️ **버전 참고 / Version Note**
> ROS2 **Humble** → C++ Task Server 필수 / C++ Task Server required
> ROS2 **Iron 이상** → Python 또는 C++ 모두 가능 / Python or C++ both available

### 세 가지 태스크 / Three Tasks

| Task ID | 이름 / Name | arm 목표 / arm target | gripper 목표 / gripper target |
|---|---|---|---|
| **0** | 홈 포지션 / Home | [0.0, 0.0, 0.0] | [-0.7, 0.7] 열림 / open |
| **1** | 픽 포지션 / Pick | [-1.14, -0.6, -0.07] | [0.0, 0.0] 닫힘 / close |
| **2** | 레스트 포지션 / Rest | [-1.57, 0.0, -0.9] | [0.0, 0.0] 닫힘 / close |

---

## 2. 액션 인터페이스 정의 / Action Interface

**`arduinobot_msgs/action/ArduinobotTask.action`**

```
# Goal: 실행할 태스크 번호 / Task number to execute
int32 task_number
---
# Result: 성공 여부 / Success flag
bool success
---
# Feedback: 완료 퍼센트 (연습용) / Completion percentage (exercise)
int32 percentage
```

### CMakeLists.txt (arduinobot_msgs) 업데이트 / Update

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "action/Fibonacci.action"
  "action/ArduinobotTask.action"   # 추가 / Add
)
```

---

## 3. Task Server C++ 구현 / C++ Implementation

**`arduinobot_remote/src/task_server.cpp`**

```cpp
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <moveit/move_group_interface/move_group_interface.h>
#include "arduinobot_msgs/action/arduinobot_task.hpp"
#include <memory>
#include <thread>

using namespace std::placeholders;

namespace arduinobot_remote
{
class TaskServer : public rclcpp::Node
{
public:
    explicit TaskServer(const rclcpp::NodeOptions &options = rclcpp::NodeOptions())
        : Node("task_server", options)
    {
        RCLCPP_INFO(get_logger(), "Starting the Server");

        // 액션 서버 생성 / Create action server
        action_server_ = rclcpp_action::create_server<arduinobot_msgs::action::ArduinobotTask>(
            this, "task_server",
            std::bind(&TaskServer::goalCallback,     this, _1, _2),
            std::bind(&TaskServer::cancelCallback,   this, _1),
            std::bind(&TaskServer::acceptedCallback, this, _1)
        );
    }

private:
    rclcpp_action::Server<arduinobot_msgs::action::ArduinobotTask>::SharedPtr action_server_;

    // Move Group 포인터 (처음 execute 호출 시 초기화)
    // Move Group pointers (initialized on first execute call)
    std::shared_ptr<moveit::planning_interface::MoveGroupInterface> arm_move_group_, gripper_move_group_;
    std::vector<double> arm_joint_goal_, gripper_joint_goal_;

    // Goal 수신 → 자동 수락 / Receive goal → auto accept
    rclcpp_action::GoalResponse goalCallback(
        const rclcpp_action::GoalUUID &uuid,
        std::shared_ptr<const arduinobot_msgs::action::ArduinobotTask::Goal> goal)
    {
        RCLCPP_INFO(get_logger(), "Received goal request with task number: %d", goal->task_number);
        (void)uuid;
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    // 취소 요청 → 로봇 정지 / Cancel request → stop robot
    rclcpp_action::CancelResponse cancelCallback(
        const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        RCLCPP_INFO(get_logger(), "Received request to cancel goal");
        if(arm_move_group_)     arm_move_group_->stop();
        if(gripper_move_group_) gripper_move_group_->stop();
        (void)goal_handle;
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    // 수락 후 별도 스레드에서 execute 실행
    // Run execute in separate thread after acceptance
    void acceptedCallback(
        const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        std::thread{std::bind(&TaskServer::execute, this, _1), goal_handle}.detach();
    }

    // 핵심 실행 함수 / Core execute function
    void execute(const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        RCLCPP_INFO(get_logger(), "Executing goal");

        // Move Group 초기화 / Initialize move groups
        if(!arm_move_group_)
            arm_move_group_ = std::make_shared<moveit::planning_interface::MoveGroupInterface>(
                shared_from_this(), "arm");
        if(!gripper_move_group_)
            gripper_move_group_ = std::make_shared<moveit::planning_interface::MoveGroupInterface>(
                shared_from_this(), "gripper");

        auto result = std::make_shared<arduinobot_msgs::action::ArduinobotTask::Result>();

        // 태스크 ID별 목표 위치 설정 / Set target by task ID
        if(goal_handle->get_goal()->task_number == 0)
        {
            arm_joint_goal_     = {0.0, 0.0, 0.0};
            gripper_joint_goal_ = {-0.7, 0.7};
        }
        else if(goal_handle->get_goal()->task_number == 1)
        {
            arm_joint_goal_     = {-1.14, -0.6, -0.07};
            gripper_joint_goal_ = {0.0, 0.0};
        }
        else if(goal_handle->get_goal()->task_number == 2)
        {
            arm_joint_goal_     = {-1.57, 0.0, -0.9};
            gripper_joint_goal_ = {0.0, 0.0};
        }
        else
        {
            RCLCPP_ERROR(get_logger(), "Invalid task number");
            return;
        }

        // 시작 상태 = 현재 상태 / Set start state to current
        arm_move_group_->setStartState(*arm_move_group_->getCurrentState());
        gripper_move_group_->setStartState(*gripper_move_group_->getCurrentState());

        // 목표 설정 + 범위 확인 / Set targets + check bounds
        bool arm_within_bounds     = arm_move_group_->setJointValueTarget(arm_joint_goal_);
        bool gripper_within_bounds = gripper_move_group_->setJointValueTarget(gripper_joint_goal_);

        if(!arm_within_bounds || !gripper_within_bounds)
        {
            RCLCPP_ERROR(get_logger(), "Target position out of boundaries");
            return;
        }

        // 궤적 계획 / Plan trajectories
        moveit::planning_interface::MoveGroupInterface::Plan arm_plan, gripper_plan;
        bool arm_plan_success =
            (arm_move_group_->plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);
        bool gripper_plan_success =
            (gripper_move_group_->plan(gripper_plan) == moveit::core::MoveItErrorCode::SUCCESS);

        // 계획 성공 시 실행 / Execute if planning succeeded
        if(arm_plan_success && gripper_plan_success)
        {
            arm_move_group_->move();
            gripper_move_group_->move();
        }
        else
        {
            RCLCPP_ERROR(get_logger(), "One or more planners failed");
            return;
        }

        // 결과 반환 / Return result
        result->success = true;
        goal_handle->succeed(result);
        RCLCPP_INFO(get_logger(), "Goal succeeded");
    }
};
} // namespace arduinobot_remote

RCLCPP_COMPONENTS_REGISTER_NODE(arduinobot_remote::TaskServer)
```

---

## 4. 빌드 설정 / Build Configuration

### CMakeLists.txt (arduinobot_remote)

```cmake
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclpy REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(arduinobot_msgs REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)

ament_python_install_package(${PROJECT_NAME})

add_library(task_server SHARED src/task_server.cpp)

ament_target_dependencies(task_server
  arduinobot_msgs rclcpp rclcpp_action
  rclcpp_components moveit_ros_planning_interface)

rclcpp_components_register_node(task_server
  PLUGIN "arduinobot_remote::TaskServer"
  EXECUTABLE task_server_node)

install(TARGETS task_server
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib   # 반드시 소문자 lib / Must be lowercase
  RUNTIME DESTINATION lib)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
```

> ⚠️ `LIBRARY DESTINATION lib` — 반드시 **소문자** `lib` 사용
> 대문자 `Lib`로 쓰면 `libtask_server.so` 를 찾지 못하는 에러 발생

### package.xml

```xml
<depend>rclcpp</depend>
<depend>rclcpp_action</depend>
<depend>rclcpp_components</depend>
<depend>arduinobot_msgs</depend>
<depend>moveit_ros_planning_interface</depend>
<exec_depend>ros2launch</exec_depend>
<exec_depend>arduinobot_moveit</exec_depend>
```

---

## 5. 런치 파일 / Launch File

**`arduinobot_remote/launch/remote_interface.launch.py`**

```python
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():

    # 런치 인자 선언 / Declare launch arguments
    is_sim_arg     = DeclareLaunchArgument('is_sim',       default_value='true')
    use_python_arg = DeclareLaunchArgument('use_python',   default_value='false')

    is_sim     = LaunchConfiguration('is_sim')
    use_python = LaunchConfiguration('use_python')

    # MoveIt2 설정 (Python 구현에서만 필요)
    # MoveIt2 config (only needed for Python implementation)
    moveit_config = (
        MoveItConfigsBuilder('arduinobot', package_name='arduinobot_moveit')
        .robot_description(...)
        .robot_description_semantic(...)
        .trajectory_execution(...)
        .to_moveit_configs()
    )

    # Python Task Server (use_python=true 일 때만 / Only when use_python=true)
    task_server_node_py = Node(
        package='arduinobot_remote',
        executable='task_server.py',
        parameters=[moveit_config.to_dict(), {'use_sim_time': is_sim}],
        condition=IfCondition(use_python)
    )

    # C++ Task Server (use_python=false 일 때만 / Only when use_python=false)
    task_server_node = Node(
        package='arduinobot_remote',
        executable='task_server_node',
        parameters=[{'use_sim_time': is_sim}],
        condition=UnlessCondition(use_python)
    )

    return LaunchDescription([
        is_sim_arg,
        use_python_arg,
        task_server_node_py,
        task_server_node,
    ])
```

### 런치 인자 / Launch Arguments

| 인자 / Argument | 기본값 / Default | 설명 / Description |
|---|---|---|
| `is_sim` | `true` | 시뮬레이션 여부 / Simulation or real robot |
| `use_python` | `false` | Python 구현 사용 여부 / Use Python or C++ |

---

## 6. 실행 순서 / Execution Order

### 터미널 1 / Terminal 1 — Gazebo

```bash
source install/setup.bash
ros2 launch arduinobot_description gazebo.launch.py
```

### 터미널 2 / Terminal 2 — ROS2 Control

```bash
source install/setup.bash
ros2 launch arduinobot_controller controller.launch.py
```

### 터미널 3 / Terminal 3 — MoveIt2

```bash
source install/setup.bash
ros2 launch arduinobot_moveit moveit.launch.py
```

### 터미널 4 / Terminal 4 — Task Server

```bash
source install/setup.bash

# C++ 실행 (Humble 포함 모든 버전 / All versions including Humble)
ros2 launch arduinobot_remote remote_interface.launch.py

# Python 실행 (Iron 이상만 / Iron and above only)
ros2 launch arduinobot_remote remote_interface.launch.py use_python:=true
```

---

## 7. 태스크 테스트 / Task Test

### 액션 서버 확인 / Verify Action Server

```bash
ros2 action list
# /task_server
```

### 태스크 실행 / Execute Tasks

```bash
# Task 0: 홈 포지션 + 그리퍼 열림 / Home + open gripper
ros2 action send_goal /task_server \
  arduinobot_msgs/action/ArduinobotTask "{task_number: 0}"

# Task 1: 픽 포지션 + 그리퍼 닫기 / Pick + close gripper
ros2 action send_goal /task_server \
  arduinobot_msgs/action/ArduinobotTask "{task_number: 1}"

# Task 2: 레스트 포지션 / Rest position
ros2 action send_goal /task_server \
  arduinobot_msgs/action/ArduinobotTask "{task_number: 2}"
```

### 실행 결과 / Expected Result

| Task | 동작 / Motion |
|---|---|
| **0** | 홈 포지션 복귀 + 그리퍼 열림 / Return to home + gripper opens |
| **1** | 픽 포지션 이동 + 그리퍼 닫힘 / Move to pick + gripper closes |
| **2** | 레스트 포지션 이동 (팔 접힘) / Move to rest (arm folded) |

---

## 📋 전체 실행 흐름 / Overall Execution Flow

```
CLI 또는 Alexa / CLI or Alexa
        │ task_number = 0 / 1 / 2
        ↓
TaskServer::goalCallback()      → Goal 수락 / Accept
TaskServer::acceptedCallback()  → 별도 스레드 시작 / Start thread
TaskServer::execute()
        │
        ├── task 0 → arm[0,0,0]          gripper[-0.7, 0.7]
        ├── task 1 → arm[-1.14,-0.6,-0.07] gripper[0,0]
        └── task 2 → arm[-1.57,0,-0.9]   gripper[0,0]
        │
        ├── setStartState() → setJointValueTarget() → plan()
        └── move() × 2 (arm + gripper)
                │
                ↓
        result->success = true
        goal_handle->succeed(result)
```

> 📌 **다음 단계 / Next Step**
> CLI 대신 **Amazon Alexa 음성 명령**으로 Task Server를 호출합니다.
> Replace CLI with **Amazon Alexa voice commands** to trigger the Task Server.
