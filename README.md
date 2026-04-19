# Arduinobot - ROS2 Robot Manipulator Simulation

## Overview
A ROS2-based robot manipulator simulation project using Gazebo.
This project implements a full pipeline from robot description to joint control,
including RGB camera simulation and ROS2 Control integration for actuating robot joints.

## Purpose
- Simulate a robot manipulator in Gazebo using ROS2
- Control robot joints (arm + gripper) via ROS2 Control framework
- Simulate an RGB camera (Raspberry Pi Camera 3 spec) attached to the robot base
- Enable real-time camera pose tuning via launch arguments (no rebuild required)

---

## Package Structure

```
arduinobot_ws/src/
├── arduinobot_description/          # Robot description (URDF/Xacro, meshes, launch)
│   ├── urdf/
│   │   ├── arduinobot.urdf.xacro       # Main robot model (links, joints)
│   │   ├── arduinobot_gazebo.xacro     # Gazebo plugin configuration
│   │   └── arduinobot_ros2_control.xacro # ROS2 Control configuration
│   ├── meshes/                      # 3D mesh files (.STL)
│   ├── launch/
│   │   ├── display.launch.py           # RViz visualization
│   │   └── gazebo.launch.py            # Gazebo simulation
│   ├── rviz/
│   │   └── display.rviz
│   ├── CMakeLists.txt
│   └── package.xml
│
└── arduinobot_controller/           # ROS2 Control configuration
    ├── config/
    │   └── arduinobot_controllers.yaml  # Controller parameters
    ├── launch/
    │   └── controller.launch.py         # Controller launch file
    ├── CMakeLists.txt
    └── package.xml
```

---

## File Descriptions

### `arduinobot_description`

#### `urdf/arduinobot.urdf.xacro`
Main robot URDF file. Defines all links and joints of the robot arm.
- Links: `base_link`, `base_plate`, `forward_drive_arm`, `horizontal_arm`, `claw_support`, `gripper_right`, `gripper_left`, `rgb_camera`
- Joints: `joint_1` ~ `joint_5` (revolute), `rgb_camera_joint` (fixed)
- Includes `arduinobot_gazebo.xacro` and `arduinobot_ros2_control.xacro`
- Supports launch arguments for camera pose tuning:
  - `cam_x`, `cam_y`, `cam_z`, `cam_roll`, `cam_pitch`, `cam_yaw`
  - `is_ignition` (default: true) — selects ROS2 Humble compatible plugins

#### `urdf/arduinobot_gazebo.xacro`
Gazebo plugin configuration file.
- Loads `ign_ros2_control-system` plugin for ROS2 Humble
- Loads `gz-sim-sensors-system` for RGB camera sensor simulation
- Configures camera sensor (resolution, FoV, update rate) matching Raspberry Pi Camera 3 spec

#### `urdf/arduinobot_ros2_control.xacro`
ROS2 Control hardware interface configuration.
- Defines `ros2_control` tag with `system` type
- Loads `IgnitionSystem` hardware plugin for Gazebo simulation
- Configures command/state interfaces (position) for each joint:
  - `joint_1`, `joint_2`, `joint_3`: arm joints (-90° ~ 90°)
  - `joint_4`: gripper right (-90° ~ 0°)
  - `joint_5`: mimics `joint_4` with multiplier `-1` (moves opposite direction)

#### `launch/display.launch.py`
Launches RViz for robot model visualization.
- Supports camera pose arguments for real-time tuning without rebuild

```bash
# Default launch
ros2 launch arduinobot_description display.launch.py

# With camera pose arguments (no rebuild needed)
ros2 launch arduinobot_description display.launch.py \
  cam_x:=0 cam_y:=0.45 cam_z:=0.2 \
  cam_roll:=-1.57 cam_pitch:=0 cam_yaw:=-1.57
```

#### `launch/gazebo.launch.py`
Launches Gazebo simulation with the robot.
- Spawns robot via `robot_description` topic
- Bridges Gazebo topics to ROS2:
  - `/clock`
  - `/rgb_camera/image_raw`
  - `/rgb_camera/camera_info`

```bash
export LIBGL_ALWAYS_SOFTWARE=1  # Required for VMware
ros2 launch arduinobot_description gazebo.launch.py
```

---

### `arduinobot_controller`

#### `config/arduinobot_controllers.yaml`
Defines three controllers:

| Controller | Type | Joints |
|---|---|---|
| `arm_controller` | JointTrajectoryController | joint_1, joint_2, joint_3 |
| `gripper_controller` | JointTrajectoryController | joint_4 |
| `joint_state_broadcaster` | JointStateBroadcaster | all joints |

All controllers use `position` command/state interface.

#### `launch/controller.launch.py`
Launches the ROS2 Control node and spawns all controllers.
- Starts `controller_manager` with robot description and YAML config
- Spawns: `arm_controller`, `gripper_controller`, `joint_state_broadcaster`

```bash
ros2 launch arduinobot_controller controller.launch.py
```

---

## How to Run

### 1. Build
```bash
cd ~/arduinobot_ws
colcon build
source install/setup.bash
```

### 2. Launch Gazebo Simulation
```bash
export LIBGL_ALWAYS_SOFTWARE=1
ros2 launch arduinobot_description gazebo.launch.py
```

### 3. Launch Controllers (new terminal)
```bash
source ~/arduinobot_ws/install/setup.bash
ros2 launch arduinobot_controller controller.launch.py
```

---

## Moving the Gripper

### Check available controllers
```bash
ros2 control list_controllers
```

### Move gripper (open)
```bash
ros2 topic pub /gripper_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_4'],
    points: [{
      positions: [-0.5],
      time_from_start: {sec: 1}
    }]
  }"
```

### Move gripper (close)
```bash
ros2 topic pub /gripper_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_4'],
    points: [{
      positions: [0.0],
      time_from_start: {sec: 1}
    }]
  }"
```

### Move arm joints
```bash
ros2 topic pub /arm_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_1', 'joint_2', 'joint_3'],
    points: [{
      positions: [0.5, 0.5, 0.5],
      time_from_start: {sec: 1}
    }]
  }"
```

---

## RGB Camera

### Verify camera topics
```bash
ros2 topic list | grep rgb_camera
ros2 topic hz /rgb_camera/image_raw
```

### Camera Specifications (Raspberry Pi Camera 3)
| Parameter | Value |
|---|---|
| Resolution | 2304 × 1296 |
| FPS | 30 |
| Horizontal FoV | 66° (1.15 rad) |
| Vertical FoV | 41° (0.71 rad) |

### Visualize in RViz
1. Add `Image` display
2. Set Fixed Frame to `base_link`
3. Set Topic to `/rgb_camera/image_raw`

---

---

# 아두이노봇 - ROS2 로봇 매니퓰레이터 시뮬레이션

## 개요
ROS2와 Gazebo를 활용한 로봇 매니퓰레이터 시뮬레이션 프로젝트입니다.
로봇 모델링부터 관절 제어, RGB 카메라 시뮬레이션까지 전체 파이프라인을 구현했습니다.

## 목적
- Gazebo에서 로봇 매니퓰레이터 시뮬레이션
- ROS2 Control 프레임워크로 로봇 관절(arm + gripper) 제어
- 라즈베리파이 카메라 3 스펙의 RGB 카메라 시뮬레이션
- launch 인자로 카메라 위치/방향 실시간 튜닝 (빌드 불필요)

---

## 패키지 구조

```
arduinobot_ws/src/
├── arduinobot_description/          # 로봇 모델 (URDF/Xacro, 메쉬, 런치)
│   ├── urdf/
│   │   ├── arduinobot.urdf.xacro       # 메인 로봇 모델 (링크, 조인트)
│   │   ├── arduinobot_gazebo.xacro     # Gazebo 플러그인 설정
│   │   └── arduinobot_ros2_control.xacro # ROS2 Control 설정
│   ├── meshes/                      # 3D 메쉬 파일 (.STL)
│   ├── launch/
│   │   ├── display.launch.py           # RViz 시각화
│   │   └── gazebo.launch.py            # Gazebo 시뮬레이션
│   ├── rviz/
│   │   └── display.rviz
│   ├── CMakeLists.txt
│   └── package.xml
│
└── arduinobot_controller/           # ROS2 Control 설정
    ├── config/
    │   └── arduinobot_controllers.yaml  # 컨트롤러 파라미터
    ├── launch/
    │   └── controller.launch.py         # 컨트롤러 런치 파일
    ├── CMakeLists.txt
    └── package.xml
```

---

## 파일 설명

### `arduinobot_description`

#### `urdf/arduinobot.urdf.xacro`
메인 로봇 URDF 파일. 로봇 팔의 모든 링크와 조인트 정의.
- 링크: `base_link`, `base_plate`, `forward_drive_arm`, `horizontal_arm`, `claw_support`, `gripper_right`, `gripper_left`, `rgb_camera`
- 조인트: `joint_1` ~ `joint_5` (revolute), `rgb_camera_joint` (fixed)
- `arduinobot_gazebo.xacro`, `arduinobot_ros2_control.xacro` include
- 카메라 위치 튜닝을 위한 launch 인자 지원:
  - `cam_x`, `cam_y`, `cam_z`, `cam_roll`, `cam_pitch`, `cam_yaw`
  - `is_ignition` (기본값: true) — ROS2 Humble 호환 플러그인 선택

#### `urdf/arduinobot_gazebo.xacro`
Gazebo 플러그인 설정 파일.
- ROS2 Humble용 `ign_ros2_control-system` 플러그인 로드
- RGB 카메라 센서 시뮬레이션을 위한 `gz-sim-sensors-system` 로드
- 라즈베리파이 카메라 3 스펙에 맞는 카메라 센서 설정 (해상도, FoV, 업데이트 주기)

#### `urdf/arduinobot_ros2_control.xacro`
ROS2 Control 하드웨어 인터페이스 설정.
- `system` 타입의 `ros2_control` 태그 정의
- Gazebo 시뮬레이션용 `IgnitionSystem` 하드웨어 플러그인 로드
- 각 조인트의 command/state 인터페이스 (position) 설정:
  - `joint_1`, `joint_2`, `joint_3`: 팔 관절 (-90° ~ 90°)
  - `joint_4`: 그리퍼 오른쪽 (-90° ~ 0°)
  - `joint_5`: `joint_4`를 multiplier `-1`로 mimic (반대 방향으로 동일하게 움직임)

#### `launch/display.launch.py`
RViz 로봇 모델 시각화 런치 파일.
- 빌드 없이 카메라 위치 실시간 튜닝 가능한 launch 인자 지원

```bash
# 기본 실행
ros2 launch arduinobot_description display.launch.py

# 카메라 위치/방향 조정 (빌드 불필요)
ros2 launch arduinobot_description display.launch.py \
  cam_x:=0 cam_y:=0.45 cam_z:=0.2 \
  cam_roll:=-1.57 cam_pitch:=0 cam_yaw:=-1.57
```

#### `launch/gazebo.launch.py`
Gazebo 시뮬레이션 런치 파일.
- `robot_description` 토픽으로 로봇 스폰
- Gazebo 토픽을 ROS2로 브릿지:
  - `/clock`
  - `/rgb_camera/image_raw`
  - `/rgb_camera/camera_info`

```bash
export LIBGL_ALWAYS_SOFTWARE=1  # VMware 환경 필수
ros2 launch arduinobot_description gazebo.launch.py
```

---

### `arduinobot_controller`

#### `config/arduinobot_controllers.yaml`
컨트롤러 3개 정의:

| 컨트롤러 | 타입 | 담당 조인트 |
|---|---|---|
| `arm_controller` | JointTrajectoryController | joint_1, joint_2, joint_3 |
| `gripper_controller` | JointTrajectoryController | joint_4 |
| `joint_state_broadcaster` | JointStateBroadcaster | 전체 조인트 |

모든 컨트롤러는 `position` command/state 인터페이스 사용.

#### `launch/controller.launch.py`
ROS2 Control 노드 및 컨트롤러 스포너 런치 파일.
- 로봇 description과 YAML 설정으로 `controller_manager` 시작
- `arm_controller`, `gripper_controller`, `joint_state_broadcaster` 스폰

```bash
ros2 launch arduinobot_controller controller.launch.py
```

---

## 실행 방법

### 1. 빌드
```bash
cd ~/arduinobot_ws
colcon build
source install/setup.bash
```

### 2. Gazebo 시뮬레이션 실행
```bash
export LIBGL_ALWAYS_SOFTWARE=1
ros2 launch arduinobot_description gazebo.launch.py
```

### 3. 컨트롤러 실행 (새 터미널)
```bash
source ~/arduinobot_ws/install/setup.bash
ros2 launch arduinobot_controller controller.launch.py
```

---

## 그리퍼 움직이기

### 컨트롤러 목록 확인
```bash
ros2 control list_controllers
```

### 그리퍼 열기
```bash
ros2 topic pub /gripper_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_4'],
    points: [{
      positions: [-0.5],
      time_from_start: {sec: 1}
    }]
  }"
```

### 그리퍼 닫기
```bash
ros2 topic pub /gripper_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_4'],
    points: [{
      positions: [0.0],
      time_from_start: {sec: 1}
    }]
  }"
```

### 팔 관절 움직이기
```bash
ros2 topic pub /arm_controller/joint_trajectory \
  trajectory_msgs/msg/JointTrajectory \
  "{
    joint_names: ['joint_1', 'joint_2', 'joint_3'],
    points: [{
      positions: [0.5, 0.5, 0.5],
      time_from_start: {sec: 1}
    }]
  }"
```

---

## RGB 카메라

### 카메라 토픽 확인
```bash
ros2 topic list | grep rgb_camera
ros2 topic hz /rgb_camera/image_raw
```

### 카메라 사양 (라즈베리파이 카메라 3)
| 파라미터 | 값 |
|---|---|
| 해상도 | 2304 × 1296 |
| FPS | 30 |
| 수평 FoV | 66° (1.15 rad) |
| 수직 FoV | 41° (0.71 rad) |

### RViz에서 카메라 영상 확인
1. `Image` 디스플레이 추가
2. Fixed Frame을 `base_link`로 설정
3. Topic을 `/rgb_camera/image_raw`로 설정
