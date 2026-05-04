# MoveIt2 C++ API 실습 / MoveIt2 C++ API Practice

---

## 📌 목차 / Table of Contents

1. [실습 개요 / Practice Overview](#1-실습-개요--practice-overview)
2. [C++ 노드 구현 / C++ Node Implementation](#2-c-노드-구현--c-node-implementation)
3. [빌드 설정 / Build Configuration](#3-빌드-설정--build-configuration)
4. [실행 순서 / Execution Order](#4-실행-순서--execution-order)
5. [실행 결과 / Expected Result](#5-실행-결과--expected-result)
6. [Python API vs C++ API 비교 / Comparison](#6-python-api-vs-c-api-비교--comparison)

---

## 1. 실습 개요 / Practice Overview

> **한국어**
> MoveIt2 C++ API를 사용해 로봇을 원하는 위치로 이동시키는 간단한 C++ 노드를 구현하고 실행합니다.
> 이 코드는 이후 더 복잡한 애플리케이션의 템플릿으로 활용됩니다.

> **English**
> Implement and run a simple C++ node that uses the MoveIt2 C++ API to move the robot to a desired position.
> This code serves as a template for building more complex applications.

> ⚠️ **버전 참고 / Version Note**
> MoveIt2 **Python API** → ROS2 Iron 이상에서만 사용 가능 / Only available on ROS2 Iron and above
> MoveIt2 **C++ API** → ROS2 Humble 포함 모든 버전 사용 가능 / Available on all versions including Humble

### 목표 동작 / Target Motion

| 관절 / Joint | 목표값 / Target | 동작 / Motion |
|---|---|---|
| joint1 (베이스 / Base) | 1.57 rad (90°) | 베이스 90° 회전 / Rotate base 90° |
| joint2, joint3 | 0.0 | 고정 / Fixed |
| joint4 (그리퍼 / Gripper) | -0.7 rad | 그리퍼 열림 / Gripper opens |
| joint5 (그리퍼 / Gripper) | 0.7 rad | 반대 방향 / Opposite direction |

---

## 2. C++ 노드 구현 / C++ Node Implementation

**`arduinobot_cpp_examples/simple_moveit_interface.cpp`**

```cpp
#include <memory>                                                    // shared_ptr 사용 / For shared_ptr
#include <rclcpp/rclcpp.hpp>                                         // ROS2 C++ 라이브러리 / ROS2 C++ library
#include <moveit/move_group_interface/move_group_interface.h>        // MoveIt2 C++ API

// 노드를 입력받아 로봇을 목표 위치로 이동시키는 함수
// Function that moves the robot to target position using MoveIt2 API
void move_robot(const std::shared_ptr<rclcpp::Node> node)
{
    // arm / gripper Move Group 접근 (SRDF에서 정의한 그룹명)
    // Access move groups (names defined in SRDF)
    auto arm_move_group     = moveit::planning_interface::MoveGroupInterface(node, "arm");
    auto gripper_move_group = moveit::planning_interface::MoveGroupInterface(node, "gripper");

    // arm 목표 관절 위치 (joint1=90°, joint2=0, joint3=0)
    // arm target joint positions (joint1=90°, joint2=0, joint3=0)
    std::vector<double> arm_joint_goal     {1.57, 0.0, 0.0};

    // gripper 목표 관절 위치 (반대 방향 회전 → 그리퍼 열림)
    // gripper target joint positions (opposite directions → gripper opens)
    std::vector<double> gripper_joint_goal {-0.7, 0.7};

    // 목표 위치 적용 + 관절 범위 내 여부 확인 (범위 초과 시 false 반환)
    // Set joint targets + check if within limits (returns false if out of limits)
    bool arm_within_bounds     = arm_move_group.setJointValueTarget(arm_joint_goal);
    bool gripper_within_bounds = gripper_move_group.setJointValueTarget(gripper_joint_goal);

    // 관절 범위 초과 시 경고 출력 후 종료
    // Warn and return if target joints are out of limits
    if(!arm_within_bounds || !gripper_within_bounds)
    {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"),
            "Target joint position(s) were outside of limits, but we will plan and clamp to the limits");
        return;
    }

    // 궤적 계획 결과를 저장할 Plan 객체 생성
    // Create Plan objects to store trajectory planning results
    moveit::planning_interface::MoveGroupInterface::Plan arm_plan;
    moveit::planning_interface::MoveGroupInterface::Plan gripper_plan;

    // 궤적 계획 실행 + 성공 여부 저장
    // Execute trajectory planning + store success result
    bool arm_plan_success =
        (arm_move_group.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);
    bool gripper_plan_success =
        (gripper_move_group.plan(gripper_plan) == moveit::core::MoveItErrorCode::SUCCESS);

    // 두 계획 모두 성공 시 실행 / Execute if both plans succeeded
    if(arm_plan_success && gripper_plan_success)
    {
        arm_move_group.move();      // arm 궤적 실행 / Execute arm trajectory
        gripper_move_group.move();  // gripper 궤적 실행 / Execute gripper trajectory
    }
    else
    {
        // 계획 실패 시 에러 출력 후 종료
        // Print error and return if planning failed
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "One or more planners failed");
        return;
    }
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);                                              // ROS2 초기화 / Initialize ROS2
    std::shared_ptr<rclcpp::Node> node =
        rclcpp::Node::make_shared("simple_moveit_interface");              // 노드 생성 / Create node
    move_robot(node);                                                      // 로봇 이동 실행 / Execute motion
    rclcpp::spin(node);                                                    // 콜백 대기 / Wait for callbacks
    rclcpp::shutdown();                                                    // ROS2 종료 / Shutdown ROS2
}
```

### 핵심 API 정리 / Key API Summary

| 함수 / Function | 설명 / Description |
|---|---|
| `MoveGroupInterface(node, "arm")` | arm Move Group 접근 / Access arm move group |
| `MoveGroupInterface(node, "gripper")` | gripper Move Group 접근 / Access gripper move group |
| `setJointValueTarget(vector)` | 목표 관절 위치 설정, bool 반환 / Set target, returns bool |
| `plan(plan_obj)` | 현재→목표 궤적 계획 / Plan trajectory current→target |
| `MoveItErrorCode::SUCCESS` | 계획 성공 여부 확인 / Check planning success |
| `move()` | 계획된 궤적 실행 / Execute planned trajectory |

---

## 3. 빌드 설정 / Build Configuration

### CMakeLists.txt

```cmake
# 의존성 추가 / Add dependency
find_package(moveit_ros_planning_interface REQUIRED)

# 실행 파일 등록 / Register executable
add_executable(simple_moveit_interface
  src/simple_moveit_interface.cpp
)

# 의존성 연결 / Link dependencies
ament_target_dependencies(simple_moveit_interface
  rclcpp
  moveit_ros_planning_interface
)

# 설치 / Install
install(TARGETS
  simple_moveit_interface
  DESTINATION lib/${PROJECT_NAME}
)
```

### package.xml

```xml
<depend>moveit_ros_planning_interface</depend>
```

### 빌드 / Build

```bash
cd ~/ros2_ws
colcon build
```

---

## 4. 실행 순서 / Execution Order

> 📌 총 5개의 터미널이 필요합니다 / 5 terminals required in total

### 터미널 1 / Terminal 1 — Gazebo 시뮬레이션

```bash
source install/setup.bash
ros2 launch arduinobot_description gazebo.launch.py
```

### 터미널 2 / Terminal 2 — ROS2 Control 컨트롤러

```bash
source install/setup.bash
ros2 launch arduinobot_controller controller.launch.py
# arm_controller + gripper_controller 정상 실행 확인
# Verify arm_controller + gripper_controller are running
```

### 터미널 3 / Terminal 3 — MoveIt2 + RViz2

```bash
source install/setup.bash
ros2 launch arduinobot_moveit moveit.launch.py
```

### 터미널 4 / Terminal 4 — API 노드 실행 (C++ 또는 Python 선택)

```bash
# ── C++ 버전 / C++ version ──────────────────────────────────
source install/setup.bash
ros2 run arduinobot_cpp_examples simple_moveit_interface

# ── Python 버전 / Python version (ROS2 Iron 이상 / Iron and above) ──
source install/setup.bash
ros2 launch arduinobot_examples simple_moveit_interface.launch.py
```

> 📌 **실행 방식 차이 / Execution difference**
> C++: `ros2 run` 으로 직접 실행 / Direct execution with `ros2 run`
> Python: 런치 파일 필요 → `ros2 launch` 사용 / Needs launch file → use `ros2 launch`

---

## 5. 실행 결과 / Expected Result

```
노드 실행 즉시 / Upon node execution:

1. MoveIt2가 현재 위치 → 목표 위치 궤적 자동 계획
   MoveIt2 auto-plans trajectory: current → target

2. arm_controller: 로봇 베이스 90° 회전
   arm_controller: Robot base rotates 90°

3. gripper_controller: 그리퍼 열림
   gripper_controller: Gripper opens
```

### 전체 실행 흐름 / Overall Execution Flow

```
simple_moveit_interface (C++ / Python)
        │
        ├── MoveGroupInterface 초기화 / Initialize
        │     ├── arm     Move Group
        │     └── gripper Move Group
        │
        ├── setJointValueTarget() → 범위 확인 / Check bounds
        │
        ├── plan() → 충돌 없는 궤적 계획 / Plan collision-free path
        │     ├── 성공 / Success → move() 실행 / Execute
        │     └── 실패 / Failed → RCLCPP_ERROR + return
        │
        └── move() → 모터 명령 전달 / Send motor commands
                │
                ↓
        ROS2 Control
        ├── arm_controller    → joint1, 2, 3
        └── gripper_controller → joint4, 5
                │
                ↓
        Gazebo 시뮬레이션에서 로봇 이동 확인
        Verify robot motion in Gazebo simulation
```

---

## 6. Python API vs C++ API 비교 / Comparison

| 항목 / Item | Python API | C++ API |
|---|---|---|
| **ROS2 버전 / Version** | Iron 이상 / Iron and above | Humble 포함 전 버전 / All versions |
| **실행 방식 / Run** | `ros2 launch` | `ros2 run` |
| **Move Group 접근 / Access** | `get_planning_component('arm')` | `MoveGroupInterface(node, "arm")` |
| **목표 설정 / Set target** | `set_joint_group_positions()` | `setJointValueTarget()` |
| **범위 확인 / Bounds check** | 별도 확인 없음 / Not explicit | bool 반환으로 확인 / Returns bool |
| **궤적 계획 / Plan** | `plan()` | `plan(plan_obj)` |
| **궤적 실행 / Execute** | `execute(trajectory)` | `move()` |
| **추가 설정 파일 / Extra config** | `planning_python_api.yaml` 필요 / Required | 불필요 / Not needed |
