# Euler ↔ Quaternion 변환 서비스 / Euler ↔ Quaternion Conversion Service

---

## 📌 목차 / Table of Contents

1. [배경 이론 / Background Theory](#1-배경-이론--background-theory)
2. [패키지 생성 / Create Package](#2-패키지-생성--create-package)
3. [라이브러리 설치 / Install Libraries](#3-라이브러리-설치--install-libraries)
4. [서비스 인터페이스 정의 / Define Service Interfaces](#4-서비스-인터페이스-정의--define-service-interfaces)
5. [노드 코드 / Node Code](#5-노드-코드--node-code)
6. [빌드 설정 / Build Configuration](#6-빌드-설정--build-configuration)
7. [빌드 및 실행 / Build & Run](#7-빌드-및-실행--build--run)

---

## 1. 배경 이론 / Background Theory

> **한국어**
> ROS2는 내부적으로 방향을 **쿼터니언**으로 표현하지만, 사람은 **오일러 각도**로 생각하는 것이 훨씬 직관적입니다.
> 따라서 두 표현법 간의 **변환 서비스**가 실용적으로 매우 유용합니다.

> **English**
> ROS2 uses **Quaternions** internally to represent orientation, but humans think more naturally in **Euler angles**.
> A **conversion service** between the two representations is therefore practically very useful.

### 두 표현법 비교 / Comparison

| 항목 / Item | 오일러 각도 / Euler Angles | 쿼터니언 / Quaternion |
|---|---|---|
| **변수 수 / Variables** | 9개 (3×3 행렬) / 9 (3×3 matrix) | 4개 / 4 |
| **직관성 / Intuitive** | ✅ 높음 / High | ❌ 낮음 / Low |
| **연산 효율 / Efficiency** | ❌ 낮음 / Low | ✅ 높음 / High |
| **역회전 계산 / Inverse rotation** | 전치 행렬 / Transpose | 부호 변환 / Sign flip |
| **ROS2 사용 / Used in ROS2** | ❌ | ✅ |

### 쿼터니언 구조 / Quaternion Structure

$$q = a + b\mathbf{i} + c\mathbf{j} + d\mathbf{k}$$

| 변수 / Variable | 명칭 / Name |
|---|---|
| **a (w)** | 스칼라 부분 / Scalar part |
| **b, c, d (x, y, z)** | 벡터 부분 / Vector part |

---

## 2. 패키지 생성 / Create Package

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake arduinobot_utils
```

> 📌 **한국어:** Python + C++ 혼용 패키지이므로 `ament_cmake` 사용.
> `ament_python`과 달리 하위 폴더가 자동 생성되지 않으므로 **직접 생성** 필요.

> 📌 **English:** Using `ament_cmake` for a mixed Python + C++ package.
> Unlike `ament_python`, subdirectories are **not auto-created** — create them manually.

### 폴더 구조 / Directory Structure

```
arduinobot_utils/
└── arduinobot_utils/        ← 패키지명과 동일 / Same as package name
    ├── __init__.py           ← 빈 파일 / Empty file
    └── angle_conversion.py   ← 노드 코드 / Node code
```

---

## 3. 라이브러리 설치 / Install Libraries

```bash
# TF 변환 모듈 / TF transformation module
sudo apt install ros-humble-tf-transformations

# Python transforms3d 라이브러리
sudo pip3 install transforms3d
```

---

## 4. 서비스 인터페이스 정의 / Define Service Interfaces

> 📌 `arduinobot_msgs` 패키지의 `srv/` 폴더에 추가합니다.
> Add to the `srv/` folder of the `arduinobot_msgs` package.

### EulerToQuaternion.srv

```
# 요청: 변환할 오일러 각도 / Request: Euler angles to convert
float64 roll
float64 pitch
float64 yaw
---
# 응답: 변환된 쿼터니언 / Response: Converted quaternion
float64 x
float64 y
float64 z
float64 w
```

### QuaternionToEuler.srv

```
# 요청: 변환할 쿼터니언 / Request: Quaternion to convert
float64 x
float64 y
float64 z
float64 w
---
# 응답: 변환된 오일러 각도 / Response: Converted Euler angles
float64 roll
float64 pitch
float64 yaw
```

### CMakeLists.txt (arduinobot_msgs) 업데이트 / Update

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/AddTwoInts.srv"
  "srv/EulerToQuaternion.srv"
  "srv/QuaternionToEuler.srv"
)
```

---

## 5. 노드 코드 / Node Code

**`arduinobot_utils/angle_conversion.py`**

```python
#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from arduinobot_msgs.srv import EulerToQuaternion, QuaternionToEuler
from tf_transformations import quaternion_from_euler, euler_from_quaternion

class AnglesConverter(Node):
    def __init__(self):
        super().__init__('angle_conversion_service_server')

        # 서비스 서버 1: 오일러 → 쿼터니언 / Server 1: Euler → Quaternion
        self.euler_to_quaternion = self.create_service(
            EulerToQuaternion,
            'euler_to_quaternion',
            self.euler_to_quaternion_callback
        )

        # 서비스 서버 2: 쿼터니언 → 오일러 / Server 2: Quaternion → Euler
        self.quaternion_to_euler = self.create_service(
            QuaternionToEuler,
            'quaternion_to_euler',
            self.quaternion_to_euler_callback
        )

        self.get_logger().info('Angle conversion services are ready')

    # 콜백 1: 오일러 → 쿼터니언 / Callback 1: Euler → Quaternion
    def euler_to_quaternion_callback(self, request, response):
        self.get_logger().info(
            f'Converting Euler: roll={request.roll}, '
            f'pitch={request.pitch}, yaw={request.yaw}'
        )
        # tf_transformations 함수로 변환 / Convert using tf_transformations
        (response.x,
         response.y,
         response.z,
         response.w) = quaternion_from_euler(
            request.roll, request.pitch, request.yaw
        )
        self.get_logger().info(
            f'Quaternion: x={response.x}, y={response.y}, '
            f'z={response.z}, w={response.w}'
        )
        return response

    # 콜백 2: 쿼터니언 → 오일러 / Callback 2: Quaternion → Euler
    def quaternion_to_euler_callback(self, request, response):
        self.get_logger().info(
            f'Converting Quaternion: x={request.x}, y={request.y}, '
            f'z={request.z}, w={request.w}'
        )
        # tf_transformations 함수로 변환 / Convert using tf_transformations
        (response.roll,
         response.pitch,
         response.yaw) = euler_from_quaternion(
            [request.x, request.y, request.z, request.w]
        )
        self.get_logger().info(
            f'Euler: roll={response.roll}, pitch={response.pitch}, '
            f'yaw={response.yaw}'
        )
        return response

def main():
    rclpy.init()
    node = AnglesConverter()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

### 핵심 변환 함수 / Key Conversion Functions

| 함수 / Function | 입력 / Input | 출력 / Output |
|---|---|---|
| `quaternion_from_euler(roll, pitch, yaw)` | 오일러 3개 / 3 Euler angles | `(x, y, z, w)` |
| `euler_from_quaternion([x, y, z, w])` | 쿼터니언 리스트 / Quaternion list | `(roll, pitch, yaw)` |

---

## 6. 빌드 설정 / Build Configuration

### CMakeLists.txt (arduinobot_utils)

```cmake
find_package(ament_cmake REQUIRED)
find_package(rclpy REQUIRED)
find_package(arduinobot_msgs REQUIRED)
find_package(ament_cmake_python REQUIRED)

# Python 패키지 선언 / Declare Python package
ament_python_install_package(${PROJECT_NAME})

# Python 노드 설치 / Install Python node
install(PROGRAMS
  ${PROJECT_NAME}/angle_conversion.py
  DESTINATION lib/${PROJECT_NAME}
)
```

### package.xml (arduinobot_utils)

```xml
<buildtool_depend>ament_cmake_python</buildtool_depend>
<depend>rclpy</depend>
<depend>arduinobot_msgs</depend>
```

---

## 7. 빌드 및 실행 / Build & Run

### 빌드 / Build

```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
```

### 노드 실행 / Run Node

```bash
ros2 run arduinobot_utils angle_conversion.py
# 출력 / Output: [INFO] Angle conversion services are ready
```

### 서비스 확인 / Verify Services

```bash
ros2 service list
# /euler_to_quaternion
# /quaternion_to_euler
```

### 서비스 직접 호출 / Call Services

```bash
# 오일러 → 쿼터니언 / Euler → Quaternion
ros2 service call /euler_to_quaternion \
  arduinobot_msgs/srv/EulerToQuaternion \
  "{roll: -0.5, pitch: 0.0, yaw: 1.5}"

# 쿼터니언 → 오일러 (단위 쿼터니언) / Quaternion → Euler (unit quaternion)
ros2 service call /quaternion_to_euler \
  arduinobot_msgs/srv/QuaternionToEuler \
  "{x: 0.0, y: 0.0, z: 0.0, w: 1.0}"
```

### 전체 흐름 / Overall Flow

```
클라이언트 / Client
    │
    ├─ roll, pitch, yaw 전송 / Send Euler angles
    │         ↓
    │   euler_to_quaternion 서버 / Server
    │   quaternion_from_euler() 호출 / Call
    │         ↓
    └─ x, y, z, w 수신 / Receive Quaternion

클라이언트 / Client
    │
    ├─ x, y, z, w 전송 / Send Quaternion
    │         ↓
    │   quaternion_to_euler 서버 / Server
    │   euler_from_quaternion() 호출 / Call
    │         ↓
    └─ roll, pitch, yaw 수신 / Receive Euler angles
```
