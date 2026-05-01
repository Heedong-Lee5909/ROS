# MoveIt2 설정 및 실행 / MoveIt2 Configuration & Launch

---

## 📌 목차 / Table of Contents

1. [패키지 생성 / Create Package](#1-패키지-생성--create-package)
2. [설정 파일 / Configuration Files](#2-설정-파일--configuration-files)
3. [Cyclone DDS 설치 / Install Cyclone DDS](#3-cyclone-dds-설치--install-cyclone-dds)
4. [런치 파일 / Launch File](#4-런치-파일--launch-file)
5. [빌드 설정 / Build Configuration](#5-빌드-설정--build-configuration)
6. [실행 순서 / Execution Order](#6-실행-순서--execution-order)
7. [RViz2 GUI 조작 / RViz2 GUI Control](#7-rviz2-gui-조작--rviz2-gui-control)
8. [전체 시스템 구조 / System Architecture](#8-전체-시스템-구조--system-architecture)
9. [디버깅 과정 / Debugging Process](#9-디버깅-과정--debugging-process)
10. [강의 코드 없이 오타 찾는 방법 / How to Find Typos Without Reference Code](#10-강의-코드-없이-오타-찾는-방법--how-to-find-typos-without-reference-code)

---

## 1. 패키지 생성 / Create Package

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake arduinobot_moveit
cd ~/ros2_ws && colcon build
```

### 폴더 구조 / Directory Structure

```
arduinobot_moveit/
├── config/
│   ├── arduinobot.srdf              — 관절 그룹 & 충돌 설정
│   ├── initial_positions.yaml       — 초기 관절 위치
│   ├── joint_limits.yaml            — 속도 & 가속도 제한
│   ├── kinematics.yaml              — IK 솔버 설정
│   ├── moveit_controllers.yaml      — 컨트롤러 연결
│   └── pilz_cartesian_limits.yaml   — 카르테시안 이동 제한
└── launch/
    └── moveit.launch.py             — MoveIt2 런치 파일
```

---

## 2. 설정 파일 / Configuration Files

### ① arduinobot.srdf — 관절 그룹 & 충돌 설정

> **한국어:** URDF와 유사한 XML 구조로, MoveIt2 전용 추가 정보를 담습니다.
> 관절 그룹 정의, 초기 상태, 인접 링크 간 충돌 검사 비활성화를 설정합니다.

> **English:** An XML file similar to URDF that contains MoveIt2-specific information.
> Defines joint groups, initial states, and disables collision checking between adjacent links.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<robot name="arduinobot">

  <!-- 관절 그룹 정의 / Define joint groups -->
  <group name="arm">
    <joint name="virtual_joint"/>
    <joint name="joint1"/>
    <joint name="joint2"/>
    <joint name="joint3"/>
    <joint name="horizontal_arm_to_close_support"/>
  </group>

  <group name="gripper">
    <joint name="joint4"/>
    <joint name="joint5"/>
  </group>

  <!-- 초기 상태 (홈 포지션) / Initial state (home position) -->
  <group_state name="home" group="arm">
    <joint name="joint1" value="0"/>
    <joint name="joint2" value="0"/>
    <joint name="joint3" value="0"/>
  </group_state>

  <group_state name="home" group="gripper">
    <joint name="joint4" value="0"/>
  </group_state>

  <!-- 충돌 검사 비활성화 (인접 링크) / Disable collision (adjacent links) -->
  <disable_collisions link1="base_link"         link2="base_plate"          reason="Adjacent"/>
  <disable_collisions link1="base_link"         link2="forward_drive_arm"   reason="Adjacent"/>
  <disable_collisions link1="base_plate"        link2="forward_drive_arm"   reason="Adjacent"/>
  <disable_collisions link1="base_plate"        link2="close_support"       reason="Adjacent"/>
  <disable_collisions link1="forward_drive_arm" link2="close_support"       reason="Adjacent"/>
  <disable_collisions link1="close_support"     link2="gripper_left"        reason="Adjacent"/>
  <disable_collisions link1="close_support"     link2="gripper_right"       reason="Adjacent"/>
  <disable_collisions link1="forward_drive_arm" link2="gripper_left"        reason="Adjacent"/>
  <disable_collisions link1="forward_drive_arm" link2="gripper_right"       reason="Adjacent"/>
  <disable_collisions link1="forward_drive_arm" link2="horizontal_arm"      reason="Adjacent"/>
  <disable_collisions link1="gripper_left"      link2="horizontal_arm"      reason="Adjacent"/>
  <disable_collisions link1="gripper_right"     link2="horizontal_arm"      reason="Adjacent"/>
  <disable_collisions link1="gripper_left"      link2="gripper_right"       reason="Adjacent"/>

</robot>
```

> ⚠️ 인접 링크는 항상 접촉 상태이므로 충돌 검사에서 제외해야 오류를 방지할 수 있습니다.
> Adjacent links are always in contact, so they must be excluded from collision checking.

---

### ② initial_positions.yaml — 초기 관절 위치

```yaml
initial_positions:
  joint1: 0
  joint2: 0
  joint3: 0
  joint4: 0
```

---

### ③ joint_limits.yaml — 속도 & 가속도 제한

```yaml
# 기본 스케일링 팩터 / Default scaling factors (10% of max)
default_velocity_scaling_factor: 0.1
default_acceleration_scaling_factor: 0.1

joint_limits:
  joint1:
    has_velocity_limits: true
    max_velocity: 10
    has_acceleration_limits: false
    max_acceleration: 0
  joint2:
    has_velocity_limits: true
    max_velocity: 10
    has_acceleration_limits: false
    max_acceleration: 0
  joint3:
    has_velocity_limits: true
    max_velocity: 10
    has_acceleration_limits: false
    max_acceleration: 0
  joint4:
    has_velocity_limits: true
    max_velocity: 10
    has_acceleration_limits: false
    max_acceleration: 0
  joint5:
    has_velocity_limits: true
    max_velocity: 10
    has_acceleration_limits: false
    max_acceleration: 0
```

> 📌 `scaling_factor: 0.1` → 최대 속도/가속도의 10%만 사용 (안전한 저속 동작)
> `scaling_factor: 0.1` → Uses only 10% of max speed/acceleration (safe slow motion)

---

### ④ kinematics.yaml — IK 솔버 설정

```yaml
arm:
  kinematics_solver: kdl_kinematics_plugin/KDLKinematicsPlugin
  kinematics_solver_search_resolution: 0.005
  kinematics_solver_timeout: 0.005
  position_only_ik: true
```

| 파라미터 / Parameter | 값 / Value | 설명 / Description |
|---|---|---|
| `kinematics_solver` | KDL Plugin | MoveIt2 기본 IK 솔버 / Built-in IK solver |
| `kinematics_solver_search_resolution` | 0.005 | 탐색 해상도 / Search resolution |
| `kinematics_solver_timeout` | 0.005 | 솔버 타임아웃 / Solver timeout |
| `position_only_ik` | true | 위치만 계산 (방향 제외) / Position only (no orientation) |

> ⚠️ `position_only_ik: true` 이유 / Reason:
> 로봇의 자유도가 3개뿐이므로 위치만 지정 가능합니다. 방향까지 지정하려면 더 많은 자유도가 필요합니다.
> The robot has only 3 DOF, so only position can be specified. Orientation requires more DOF.

---

### ⑤ moveit_controllers.yaml — 컨트롤러 연결

> **한국어:** MoveIt2와 ROS2 Control 라이브러리를 연결하는 설정입니다.
> **English:** Connects MoveIt2 with the ROS2 Control library.

```yaml
moveit_controller_manager: moveit_simple_controller_manager/MoveItSimpleControllerManager
moveit_simple_controller_manager:
  controller_names:
    - arm_controller
    - gripper_controller

  arm_controller:
    action_ns: follow_joint_trajectory
    type: FollowJointTrajectory
    default: true
    joints:
      - joint1
      - joint2
      - joint3

  gripper_controller:
    action_ns: follow_joint_trajectory
    type: FollowJointTrajectory
    default: true
    joints:
      - joint4
      - joint5
```

> ⚠️ **중요 / Important:** `arm_controller`, `gripper_controller` 는 반드시 `moveit_simple_controller_manager` 안에 들여쓰기되어야 합니다. 밖에 있으면 MoveIt2가 컨트롤러를 인식하지 못합니다.
> `arm_controller` and `gripper_controller` must be indented inside `moveit_simple_controller_manager`. If placed outside, MoveIt2 cannot recognize the controllers.

| 컨트롤러 / Controller | 담당 관절 / Joints |
|---|---|
| **arm_controller** | joint1, joint2, joint3 |
| **gripper_controller** | joint4, joint5 |

---

### ⑥ pilz_cartesian_limits.yaml — 카르테시안 이동 제한

```yaml
cartesian_limits:
  max_trans_vel: 1.0      # 최대 병진 속도 / Max translational velocity
  max_trans_acc: 2.25     # 최대 병진 가속도 / Max translational acceleration
  max_trans_dec: -5.0     # 최대 병진 감속도 / Max translational deceleration
  max_rot_vel: 1.57       # 최대 회전 속도 (≈90°/s) / Max rotational velocity
```

> ⚠️ `max_trans_dec` 는 반드시 `-5.0` 처럼 소수점을 붙여야 합니다. `-5` (정수)로 쓰면 타입 오류가 발생합니다.
> `max_trans_dec` must be written as `-5.0` (float). Writing `-5` (integer) causes a type error.

---

## 3. Cyclone DDS 설치 / Install Cyclone DDS

> **한국어:** MoveIt2는 ROS2 기본 DDS와 호환성 문제가 있으므로 Cyclone DDS로 교체합니다.
> **English:** MoveIt2 has issues with the default ROS2 DDS, so switch to Cyclone DDS.

```bash
# 설치 / Install
sudo apt install ros-humble-rmw-cyclonedds-cpp

# ~/.bashrc에 영구 설정 / Add to ~/.bashrc permanently
echo "export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp" >> ~/.bashrc
source ~/.bashrc
```

---

## 4. 런치 파일 / Launch File

`arduinobot_moveit/launch/moveit.launch.py`

```python
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():

    # is_sim 인자 / is_sim argument (true: 시뮬레이션, false: 실제 로봇)
    is_sim_arg = DeclareLaunchArgument('is_sim', default_value='true')
    is_sim = LaunchConfiguration('is_sim')

    # MoveIt2 설정 빌드 / Build MoveIt2 configuration
    moveit_config = (
        MoveItConfigsBuilder('arduinobot', package_name='arduinobot_moveit')
        .robot_description(
            file_path=os.path.join(
                get_package_share_directory('arduinobot_description'),
                'urdf', 'arduinobot.urdf.xacro'
            )
        )
        .robot_description_semantic(
            file_path=os.path.join('config', 'arduinobot.srdf')
        )
        .trajectory_execution(
            file_path=os.path.join('config', 'moveit_controllers.yaml')
        )
        .to_moveit_configs()
    )

    # Move Group 노드 (MoveIt2 핵심) / Move Group node (MoveIt2 core)
    move_group_node = Node(
        package='moveit_ros_move_group',
        executable='move_group',
        output='screen',
        parameters=[
            moveit_config.to_dict(),
            {'use_sim_time': is_sim},
            {'publish_robot_description_semantic': True},
        ],
        arguments=['--ros-args', '--log-level', 'info']
    )

    # RViz2 설정 경로 / RViz2 config path
    rviz_config = os.path.join(
        get_package_share_directory('arduinobot_moveit'),
        'config', 'moveit.rviz'
    )

    # RViz2 노드 / RViz2 node
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', rviz_config],
        parameters=[
            moveit_config.robot_description,
            moveit_config.robot_description_semantic,
            moveit_config.robot_description_kinematics,
            moveit_config.joint_limits,
        ]
    )

    return LaunchDescription([
        is_sim_arg,
        move_group_node,
        rviz_node,
    ])
```

### 런치 파일 구성 요소 / Launch File Components

| 구성 요소 / Component | 역할 / Role |
|---|---|
| `is_sim_arg` | 시뮬레이션 / 실제 로봇 선택 / Sim or real robot selection |
| `MoveItConfigsBuilder` | 설정 파일들을 MoveIt2에 전달 / Pass config files to MoveIt2 |
| `move_group_node` | MoveIt2 핵심 미들웨어 / MoveIt2 core middleware |
| `rviz_node` | GUI 인터페이스 / Graphical interface |

---

## 5. 빌드 설정 / Build Configuration

### CMakeLists.txt

```cmake
install(
  DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)
```

### package.xml

```xml
<exec_depend>ros2launch</exec_depend>
<exec_depend>rviz2</exec_depend>
<exec_depend>moveit_configs_utils</exec_depend>
```

---

## 6. 실행 순서 / Execution Order

### 터미널 1 / Terminal 1 — Gazebo 시뮬레이션

```bash
source install/setup.bash
ros2 launch arduinobot_description gazebo.launch.py
```

### 터미널 2 / Terminal 2 — ROS2 Control 컨트롤러

```bash
source install/setup.bash
ros2 launch arduinobot_controller controller.launch.py
```

### 터미널 3 / Terminal 3 — MoveIt2 + RViz2

```bash
source install/setup.bash
ros2 launch arduinobot_moveit moveit.launch.py
```

> 📌 **첫 실행 시 / First launch:**
> `moveit.rviz` 파일이 없으므로 빈 RViz2 창이 열립니다. 플러그인 추가 후 저장하면 다음 실행부터 자동 로드됩니다.
> No `moveit.rviz` file exists yet, so an empty RViz2 window opens. Save after adding plugins for auto-load next time.

---

## 7. RViz2 GUI 조작 / RViz2 GUI Control

### 초기 설정 / Initial Setup

```
1. Fixed Frame  →  "world" 로 변경 / Change to "world"
2. Add  →  MoveIt2 Visualization  →  MotionPlanning 플러그인 추가 / Add plugin
3. Context 탭  →  Planning Library  →  OMPL 선택 / Select OMPL
4. "Approx. IK Solutions" 체크박스 활성화 / Enable checkbox (속도 향상 / faster)
```

> OMPL 미설치 시 / If OMPL not installed:
> ```bash
> sudo apt install ros-humble-moveit-planners-ompl
> ```

### 로봇 팔 이동 / Move Robot Arm

| 단계 / Step | 동작 / Action |
|---|---|
| 1 | Planning Group → **arm** 선택 / Select |
| 2 | 주황색 구체 드래그 → 목표 위치 설정 / Drag orange sphere → set goal |
| 3 | **[Plan and Execute]** 클릭 / Click |
| 4 | 빨간색 = 주황색 확인 (현재 = 목표) / Red = Orange (current = goal) |

### 그리퍼 이동 / Move Gripper

| 단계 / Step | 동작 / Action |
|---|---|
| 1 | Planning Group → **gripper** 선택 / Select |
| 2 | Goal State → **Random Valid** 선택 / Select |
| 3 | **[Plan and Execute]** 클릭 / Click |
| 4 | 그리퍼 열림/닫힘 확인 / Verify gripper open/close |

### 상태 표시 / State Visualization

| 색상 / Color | 의미 / Meaning |
|---|---|
| 🟠 주황색 / Orange | 목표 상태 / Goal state |
| 🔴 빨간색 / Red | 현재 상태 / Current state |

---

## 8. 전체 시스템 구조 / System Architecture

```
RViz2 GUI
(목표 위치 입력 / Goal pose input)
        │
        ▼
  [ Move Group ]
        │
        ├── IK Solver (KDL)
        │     └── 관절 각도 계산 / Compute joint angles
        │
        └── OMPL Planner
              └── 충돌 없는 궤적 계획 / Plan collision-free trajectory
                        │
                        ▼
                  ROS2 Control
                  ├── arm_controller    → joint1, 2, 3
                  └── gripper_controller → joint4, 5
                        │
                        ▼
              Gazebo 시뮬레이션 / Real Robot
```

---

## 📋 설정 파일 요약 / Configuration Files Summary

| 파일 / File | 역할 / Role |
|---|---|
| `arduinobot.srdf` | 관절 그룹 & 충돌 비활성화 / Joint groups & collision disabling |
| `initial_positions.yaml` | 초기 관절 위치 (모두 0) / Initial joint positions (all 0) |
| `joint_limits.yaml` | 속도/가속도 제한 / Velocity & acceleration limits |
| `kinematics.yaml` | KDL IK 솔버 설정 / KDL IK solver config |
| `moveit_controllers.yaml` | ROS2 Control 연결 / ROS2 Control connection |
| `pilz_cartesian_limits.yaml` | 카르테시안 이동 제한 / Cartesian motion limits |

---

## 9. 디버깅 과정 / Debugging Process

### 🔴 발생한 문제 / Issues

1. **Arm Controller** — 경로 계획은 성공하지만 실제 실행 실패 / Path planning succeeds but execution fails
2. **Gripper Controller** — 실행 성공으로 나오지만 실제로 안 움직임 / Reports success but gripper doesn't actually move

---

### 🔍 Arm Controller 디버깅 / Arm Controller Debugging

#### 원인 / Root Cause

> **한국어:** `moveit_controllers.yaml` 에서 `arm_controller`, `gripper_controller` 가 `moveit_simple_controller_manager` 밖에 있어서 MoveIt이 컨트롤러를 인식하지 못했어요.
> **English:** `arm_controller` and `gripper_controller` were placed outside `moveit_simple_controller_manager`, so MoveIt could not recognize them.

```yaml
# ❌ 잘못된 구조 / Wrong structure
moveit_simple_controller_manager:
  controller_names:
    - arm_controller

arm_controller:        # ← 최상위 레벨 (잘못됨) / Top level (wrong)
  action_ns: ...

# ✅ 올바른 구조 / Correct structure
moveit_simple_controller_manager:
  controller_names:
    - arm_controller
  arm_controller:      # ← 들여쓰기 필요 / Indentation required
    action_ns: ...
```

#### 디버깅 명령어 / Debug Commands

```bash
# 1. 컨트롤러 활성화 상태 확인 / Check controller activation status
ros2 control list_controllers
# 목적 / Purpose: arm_controller, gripper_controller가 active 상태인지 확인
#                 Verify arm_controller and gripper_controller are active

# 2. action server 존재 여부 확인 / Check action server existence
ros2 action list
# 목적 / Purpose: /arm_controller/follow_joint_trajectory가 있는지 확인
#                 Verify /arm_controller/follow_joint_trajectory exists

# 3. action server 상세 정보 확인 / Check action server details
ros2 action info /arm_controller/follow_joint_trajectory
# 목적 / Purpose: action server가 실제로 살아있는지 확인
#                 Verify action server is actually running

# 4. MoveIt 파라미터 목록 확인 / Check MoveIt parameter list
ros2 param list /move_group | grep controller
# 목적 / Purpose: MoveIt이 컨트롤러 설정을 제대로 읽었는지 확인
#                 Verify MoveIt loaded controller configuration correctly

# 5. 컨트롤러 이름 목록 확인 / Check controller name list
ros2 param get /move_group moveit_simple_controller_manager.controller_names
# 목적 / Purpose: MoveIt이 인식한 컨트롤러 목록 확인
#                 Check the list of controllers recognized by MoveIt

# 6. action_ns 값 확인 / Check action_ns value
ros2 param get /move_group arm_controller.action_ns
# 목적 / Purpose: action 네임스페이스가 올바르게 설정됐는지 확인
#                 Verify action namespace is correctly configured

# 7. use_sim_time 일치 여부 확인 / Check use_sim_time consistency
ros2 param get /arm_controller use_sim_time
ros2 param get /move_group use_sim_time
# 목적 / Purpose: 시뮬레이션 시간 설정이 일치하는지 확인
#                 Verify simulation time settings match
```

---

### 🔍 Gripper Controller 디버깅 / Gripper Controller Debugging

#### 원인 / Root Cause

> **한국어:** `arduinobot_controllers.yaml` 에서 파라미터 이름 오타(`open_loop_controller` → `open_loop_control`)로 인해 Gazebo가 joint_4 position 값을 읽지 못했어요.
> **English:** A typo in `arduinobot_controllers.yaml` (`open_loop_controller` → `open_loop_control`) caused Gazebo to fail reading joint_4 position values.

```yaml
# ❌ 잘못된 것 / Wrong
open_loop_controller: true   # controller (r 있음 / has extra 'r')

# ✅ 올바른 것 / Correct
open_loop_control: true      # control (r 없음 / no extra 'r')
```

#### 디버깅 명령어 / Debug Commands

```bash
# 1. joint_states 토픽으로 관절 위치 변화 확인
#    Check joint position changes via joint_states topic
ros2 topic echo /joint_states
# 목적 / Purpose: Plan & Execute 전후로 joint_4 값이 변하는지 확인
#                 Verify joint_4 value changes before/after Plan & Execute

# 2. gripper controller에 직접 명령 전송
#    Send command directly to gripper controller
ros2 action send_goal /gripper_controller/follow_joint_trajectory \
  control_msgs/action/FollowJointTrajectory \
  "{trajectory: {joint_names: [joint_4], points: [{positions: [-0.5], time_from_start: {sec: 1}}]}}"
# 목적 / Purpose: MoveIt 없이 직접 컨트롤러에 명령을 보내 컨트롤러 자체 문제인지 확인
#                 Send command directly without MoveIt to isolate controller issues

# 3. hardware interface 상태 확인 / Check hardware interface status
ros2 control list_hardware_interfaces
# 목적 / Purpose: joint_4의 command interface가 claimed 상태인지 확인
#                 Verify joint_4 command interface is in 'claimed' state

# 4. gripper controller 상태 토픽 확인
#    Check gripper controller state topic
ros2 topic echo /gripper_controller/controller_state
# 목적 / Purpose: 실제 position과 desired position 값 비교
#                 Compare actual position vs desired position values

# 5. gripper controller 파라미터 목록 확인
#    Check gripper controller parameter list
ros2 param list /gripper_controller
# 목적 / Purpose: 실제로 로드된 파라미터 이름 확인 (오타 감지)
#                 Check actually loaded parameter names (detect typos)
```

---

### 📋 발견된 오타 목록 / List of Typos Found

> **한국어:** 아래 오타들은 ROS2가 해당 파라미터를 조용히 무시하기 때문에 에러 메시지가 없어서 찾기 어렵습니다.
> **English:** The typos below are hard to find because ROS2 silently ignores unknown parameters without error messages.

| 파일 / File | 잘못된 것 / Wrong | 올바른 것 / Correct | 증상 / Symptom |
|---|---|---|---|
| `arduinobot.srdf` | `<disable_collision>` | `<disable_collisions>` | 충돌 감지 오류 / Collision detection error |
| `joint_limits.yaml` | `default_velocity_scailing_factor` | `default_velocity_scaling_factor` | 속도 제한 미적용 / Speed limit not applied |
| `pilz_cartesian_limits.yaml` | `max_trans_dec: -5` | `max_trans_dec: -5.0` | 타입 오류로 move_group 크래시 / Type error crashes move_group |
| `moveit_controllers.yaml` | 컨트롤러 들여쓰기 오류 / Indentation error | `moveit_simple_controller_manager` 안으로 이동 / Move inside | 컨트롤러 인식 실패 / Controller not recognized |
| `arduinobot_controllers.yaml` | `open_loop_controller: true` | `open_loop_control: true` | 그리퍼 미작동 / Gripper not moving |

---

## 10. 강의 코드 없이 오타 찾는 방법 / How to Find Typos Without Reference Code

### 핵심 원칙 / Key Principle

> **한국어:** 파라미터 이름 오타는 ROS2가 해당 파라미터를 조용히 무시하기 때문에 에러 메시지가 없어서 찾기 어렵습니다. 아래 방법들로 실제 파라미터 이름을 직접 확인할 수 있어요.
> **English:** Parameter name typos are hard to find because ROS2 silently ignores unknown parameters without error messages. Use the methods below to directly verify actual parameter names.

---

### 방법 1. 실제 파라미터 목록 확인 / Method 1: Check Actual Parameter List

```bash
ros2 param list /gripper_controller
ros2 param list /arm_controller
ros2 param list /move_group
```

> **한국어:** 노드에 실제로 로드된 파라미터 이름 목록을 확인해요. yaml 파일에서 오타가 있으면 해당 파라미터가 목록에 없거나 다른 이름으로 나타나요.
> **English:** Shows the list of actually loaded parameter names. If there's a typo in yaml, the parameter won't appear in the list or will show under a different name.

---

### 방법 2. 특정 파라미터 값 직접 조회 / Method 2: Query Specific Parameter Value

```bash
# ✅ 올바른 이름으로 조회 → 값이 나옴 / Correct name → returns value
ros2 param get /gripper_controller open_loop_control

# ❌ 오타인 이름으로 조회 → 에러 발생 / Typo name → error occurs
ros2 param get /gripper_controller open_loop_controller
```

> **한국어:** 존재하지 않는 파라미터 이름으로 조회하면 에러가 나서 오타를 발견할 수 있어요.
> **English:** Querying a non-existent parameter name returns an error, helping you detect typos.

---

### 방법 3. 설치된 ROS2 패키지 소스에서 검색 / Method 3: Search Installed ROS2 Package Source

```bash
# 실제 파라미터 이름 검색 / Search for actual parameter name
grep -r "open_loop_control" /opt/ros/humble/
```

> **한국어:** 설치된 ROS2 패키지 내에서 실제 파라미터 이름을 검색해서 올바른 이름을 확인할 수 있어요.
> **English:** Search within installed ROS2 packages to find the correct parameter name.

---

### 방법 4. 디버그 레벨 로그로 실행 / Method 4: Run with Debug Log Level

```bash
ros2 launch arduinobot_moveit moveit.launch.py --log-level debug
```

> **한국어:** debug 레벨로 실행하면 파라미터 로딩 과정에서 알 수 없는 파라미터에 대한 경고가 출력되는 경우가 있어요.
> **English:** Running with debug level may output warnings about unknown parameters during the parameter loading process.

---

### 방법 5. 공식 문서 확인 / Method 5: Check Official Documentation

> **한국어:** 각 패키지의 공식 ROS2 문서나 GitHub 소스코드에서 파라미터 이름을 직접 확인할 수 있어요.
> **English:** Check the official ROS2 documentation or GitHub source code for each package to verify parameter names.

```
# joint_trajectory_controller 공식 파라미터 목록 / Official parameter list
https://control.ros.org/humble/doc/ros2_controllers/joint_trajectory_controller/doc/parameters.html
```

---

### 방법 요약 / Method Summary

| 방법 / Method | 명령어 / Command | 언제 사용 / When to Use |
|---|---|---|
| 파라미터 목록 확인 / Check parameter list | `ros2 param list /노드명` | 어떤 파라미터가 로드됐는지 전체 확인 / Check all loaded parameters |
| 특정 파라미터 조회 / Query specific parameter | `ros2 param get /노드명 파라미터명` | 특정 파라미터 오타 여부 확인 / Check specific parameter typo |
| 소스 검색 / Search source | `grep -r "파라미터명" /opt/ros/humble/` | 올바른 파라미터 이름 검색 / Find correct parameter name |
| 디버그 로그 / Debug log | `--log-level debug` | 전반적인 파라미터 로딩 문제 확인 / Check overall parameter loading issues |
| 공식 문서 / Official docs | ROS2 공식 문서 참고 / Refer to ROS2 official docs | 패키지별 파라미터 명세 확인 / Check package-specific parameter specs |

> 💡 **가장 빠른 방법 / Fastest Method:** `ros2 param list /노드명` 으로 실제 로드된 파라미터 목록을 확인하고, yaml 파일과 비교하는 게 가장 현실적이에요!
> Use `ros2 param list /node_name` to check actually loaded parameters and compare with your yaml file!
