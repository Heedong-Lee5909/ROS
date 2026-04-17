# ROS2 Arduinobot - Assignment 2: RGB Camera Simulation in Gazebo
# ROS2 아두이노봇 - 과제 2: Gazebo RGB 카메라 시뮬레이션

---

## 📌 Purpose / 과제 목적

**[EN]**
The goal of this assignment is to simulate an RGB camera sensor attached to the robot frame in Gazebo.
By doing so, Gazebo publishes a simulated video stream as a ROS 2 topic (`/image_raw`), which can be used to develop and test computer vision algorithms without requiring real hardware.

**[KR]**
이 과제의 목적은 Gazebo 시뮬레이션 환경에서 로봇 프레임에 부착된 RGB 카메라 센서를 시뮬레이션하는 것입니다.
이를 통해 Gazebo는 시뮬레이션된 카메라 영상을 ROS 2 토픽(`/image_raw`)으로 발행하며, 실제 하드웨어 없이도 컴퓨터 비전 알고리즘을 개발하고 테스트할 수 있습니다.

---

## 📁 Folder Structure / 폴더 구조

```
arduinobot_ws/
└── src/
    ├── arduinobot_description/        # Robot description package / 로봇 모델 패키지
    │   ├── launch/
    │   │   ├── display.launch.py      # RViz2 visualization / RViz2 시각화 실행
    │   │   ├── gazebo.launch.py       # Gazebo simulation launch / Gazebo 시뮬레이션 실행 (수정됨)
    │   │   ├── camera.launch.xml      # Camera launch file / 카메라 실행 파일
    │   │   └── rgbd_camera.launch.py  # RGB-D camera launch / RGB-D 카메라 실행
    │   ├── meshes/                    # 3D model files (STL) / 3D 모델 파일
    │   ├── rviz/
    │   │   └── display.rviz          # RViz2 configuration / RViz2 설정 파일
    │   └── urdf/
    │       ├── arduinobot.urdf.xacro         # Main robot model / 메인 로봇 모델 (수정됨)
    │       ├── arduino_gazebo.xacro          # Gazebo plugins & sensor config / Gazebo 플러그인 및 센서 설정 (추가됨)
    │       └── arduinobot_ros2_control.xacro # ROS2 Control configuration / ROS2 제어 설정
    ├── arduinobot_cpp_examples/       # C++ example package / C++ 예제 패키지
    └── arduinobot_py_examples/        # Python example package / Python 예제 패키지
```

---

## 🔧 What Was Modified / 수정 및 추가 내용

### 1. `arduinobot.urdf.xacro` (Modified / 수정)

**[EN]**
- Added `is_ignition` argument to distinguish between ROS 2 Humble (Ignition Gazebo) and newer versions
- Added `xacro:include` to import `arduino_gazebo.xacro`
- Added `rgb_camera` link with `<visual>`, `<collision>`, and `<inertial>` tags
- Added `rgb_camera_joint` to attach the camera to the `base_link`

**[KR]**
- ROS 2 Humble(Ignition Gazebo)과 상위 버전을 구분하기 위한 `is_ignition` 인자 추가
- `arduino_gazebo.xacro`를 불러오기 위한 `xacro:include` 추가
- `<visual>`, `<collision>`, `<inertial>` 태그를 포함한 `rgb_camera` 링크 추가
- 카메라를 `base_link`에 부착하기 위한 `rgb_camera_joint` 추가

---

### 2. `arduino_gazebo.xacro` (Added / 추가)

**[EN]**
- Added ROS 2 Control plugin (`ign_ros2_control` for Humble, `gz_ros2_control` for Iron+)
- Added Sensors system plugin to enable camera simulation in Gazebo
- Configured RGB camera sensor with the following specs matching a **Raspberry Pi Camera 3**:

**[KR]**
- ROS 2 Control 플러그인 추가 (Humble용 `ign_ros2_control`, Iron 이상용 `gz_ros2_control`)
- Gazebo에서 카메라 시뮬레이션을 활성화하기 위한 Sensors 시스템 플러그인 추가
- **Raspberry Pi Camera 3** 하드웨어 스펙에 맞춰 RGB 카메라 센서 설정:

| Parameter / 파라미터 | Value / 값 |
|---------------------|-----------|
| Resolution / 해상도 | 2304 × 1296 |
| FPS | 30 |
| Horizontal FoV / 수평 화각 | 1.15 rad (66°) |
| Vertical FoV / 수직 화각 | 0.71 rad (41°) |
| Topic | `/image_raw` |
| Frame ID | `/rgb_camera` |

---

### 3. `gazebo.launch.py` (Modified / 수정)

**[EN]**
- Added `ros_gz_bridge` node to bridge Gazebo topics to ROS 2:
  - `/clock` → synchronize simulation time
  - `/image_raw` → RGB camera image stream
  - `/camera_info` → camera calibration info

**[KR]**
- Gazebo 토픽을 ROS 2로 브리지하기 위한 `ros_gz_bridge` 노드 추가:
  - `/clock` → 시뮬레이션 시간 동기화
  - `/image_raw` → RGB 카메라 이미지 스트림
  - `/camera_info` → 카메라 캘리브레이션 정보

---

## 💡 Key Concepts / 주요 개념 설명

### URDF / XACRO
**[EN]** URDF (Unified Robot Description Format) defines the robot's structure using XML. XACRO is a macro language that extends URDF, allowing variables, conditionals, and file includes to make the model more modular and reusable.

**[KR]** URDF(Unified Robot Description Format)는 XML을 사용하여 로봇의 구조를 정의합니다. XACRO는 URDF를 확장한 매크로 언어로, 변수, 조건문, 파일 포함 기능을 통해 모델을 더 모듈화하고 재사용 가능하게 만들어줍니다.

### Gazebo Sensor Plugin
**[EN]** The `<sensor>` tag inside a `<gazebo>` block activates sensor simulation in Gazebo. The Sensors system plugin (`ignition-gazebo-sensors-system`) must be loaded to enable this functionality.

**[KR]** `<gazebo>` 블록 내의 `<sensor>` 태그는 Gazebo에서 센서 시뮬레이션을 활성화합니다. 이 기능을 사용하려면 Sensors 시스템 플러그인(`ignition-gazebo-sensors-system`)이 반드시 로드되어야 합니다.

### ROS-Gazebo Bridge
**[EN]** `ros_gz_bridge` is a ROS 2 package that creates a communication bridge between ROS 2 and Gazebo. It allows topics published by Gazebo (e.g., camera images) to be available as ROS 2 topics.

**[KR]** `ros_gz_bridge`는 ROS 2와 Gazebo 사이의 통신 브리지를 만드는 ROS 2 패키지입니다. Gazebo에서 발행하는 토픽(예: 카메라 이미지)을 ROS 2 토픽으로 사용할 수 있게 해줍니다.

### is_ignition Flag
**[EN]** ROS 2 Humble uses Ignition Gazebo (now called Gazebo Classic), while ROS 2 Iron and above use the new Gazebo (Harmonic). The `is_ignition` flag in the XACRO file allows the same model to be used across different ROS 2 versions by loading the appropriate plugins.

**[KR]** ROS 2 Humble은 Ignition Gazebo(현재 Gazebo Classic으로 불림)를 사용하고, ROS 2 Iron 이상은 새로운 Gazebo(Harmonic)를 사용합니다. XACRO 파일의 `is_ignition` 플래그를 통해 적절한 플러그인을 로드하여 동일한 모델을 다양한 ROS 2 버전에서 사용할 수 있습니다.

---

## 🚀 How to Run / 실행 방법

```bash
# Build workspace / 워크스페이스 빌드
cd ~/arduinobot_ws
colcon build
source install/setup.bash

# Launch Gazebo simulation / Gazebo 시뮬레이션 실행
ros2 launch arduinobot_description gazebo.launch.py

# Check camera topics / 카메라 토픽 확인
ros2 topic list

# Visualize in RViz2 / RViz2에서 시각화
rviz2
```

---

## ✅ Expected Results / 기대 결과

**[EN]**
After launching the simulation, the following topics should be available:
- `/image_raw` - RGB camera image stream
- `/camera_info` - Camera calibration information
- `/clock` - Simulation time

The camera feed can be visualized in RViz2 by adding an **Image** display and selecting the `/image_raw` topic.

**[KR]**
시뮬레이션 실행 후 다음 토픽들이 사용 가능해야 합니다:
- `/image_raw` - RGB 카메라 이미지 스트림
- `/camera_info` - 카메라 캘리브레이션 정보
- `/clock` - 시뮬레이션 시간

RViz2에서 **Image** 디스플레이를 추가하고 `/image_raw` 토픽을 선택하면 카메라 화면을 시각화할 수 있습니다.

---

## 🛠️ Development Environment / 개발 환경

| Item / 항목 | Version / 버전 |
|------------|---------------|
| ROS 2 | Humble |
| Gazebo | Ignition Gazebo 6 |
| OS | Ubuntu 22.04 |
| Python | 3.10 |
