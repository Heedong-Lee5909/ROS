# ROS2 Service — 이론 및 실습 정리 / Theory & Practice

## 📌 목차 / Table of Contents

1. [서비스란? / What is a Service?](#서비스란--what-is-a-service)
2. [통신 방식 비교 / Communication Comparison](#통신-방식-비교--communication-comparison)
3. [메시지 인터페이스 정의 / Message Interface Definition](#메시지-인터페이스-정의--message-interface-definition)
4. [서비스 서버 구현 / Service Server Implementation](#서비스-서버-구현--service-server-implementation)
5. [서비스 클라이언트 구현 / Service Client Implementation](#서비스-클라이언트-구현--service-client-implementation)
6. [빌드 및 실행 / Build & Run](#빌드-및-실행--build--run)
7. [CLI 명령어 / CLI Commands](#cli-명령어--cli-commands)

---

## 서비스란? / What is a Service?

> **한국어**
> ROS2에서 노드 간 통신 방식 중 하나로, **요청(Request) — 응답(Response)** 구조를 사용합니다.
> 같은 기능을 여러 노드에 중복 구현하는 대신, **서비스 서버 하나**에 구현하고 여러 노드가 공유합니다.

> **English**
> A Service is a communication protocol in ROS2 that uses a **Request — Response** pattern.
> Instead of duplicating the same functionality across multiple nodes, it is implemented once in a **Service Server** and shared by any node that needs it.

### 구성 요소 / Components

| 역할 / Role | 이름 / Name | 설명 / Description |
|---|---|---|
| **서비스 서버** / Service Server | Server Node | 기능을 제공하는 노드 / Node that provides the functionality |
| **서비스 클라이언트** / Service Client | Client Node | 기능을 요청하는 노드 / Node that requests the functionality |

### 통신 흐름 / Communication Flow

```
클라이언트 / Client                   서버 / Server
        │                                  │
        │──── Request (요청 + 데이터) ────→ │
        │     (Request + Data)              │
        │                                  │ 처리 중 / Processing...
        │   (다른 작업 계속 가능)            │
        │   (Can continue other tasks)      │
        │                                  │
        │←─── Response (결과 반환) ──────── │
        │     (Result returned)             │
        │                                  │
        │ 결과를 받아 다음 동작 수행          │
        │ (Use result for next action)      │
```

---

## 통신 방식 비교 / Communication Comparison

| 항목 / Item | Publisher-Subscriber | Service |
|---|---|---|
| **통신 방향 / Direction** | 단방향 / One-way | 양방향 / Two-way |
| **응답 여부 / Response** | 없음 / None | 있음 / Yes |
| **주요 용도 / Use Case** | 센서 데이터 스트림 / Sensor data stream | 기능 요청 및 결과 반환 / Function call & result |
| **데이터 흐름 / Data flow** | 지속적 발행 / Continuous publish | 요청 시에만 / On demand |

---

## 메시지 인터페이스 정의 / Message Interface Definition

### 패키지 생성 / Create Package

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake arduinobot_msgs
```

> 📌 **관례 / Convention**
> 사용자 정의 메시지는 기능 패키지와 **분리된 전용 패키지**에 보관합니다.
> User-defined messages should be placed in a **separate dedicated package** from functional packages.

### 인터페이스 파일 / Interface File

`arduinobot_msgs/srv/AddTwoInts.srv`

```
# 요청 메시지 / Request message (Client → Server)
int64 a
int64 b
---
# 응답 메시지 / Response message (Server → Client)
int64 sum
```

> `---` 위 / Above: **Request** &nbsp;|&nbsp; `---` 아래 / Below: **Response**

### CMakeLists.txt

```cmake
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/AddTwoInts.srv"
)
```

### package.xml

```xml
<depend>std_msgs</depend>
<build_depend>rosidl_default_generators</build_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

---

## 서비스 서버 구현 / Service Server Implementation

`arduinobot_examples/simple_service_server.py`

```python
import rclpy
from rclpy.node import Node
from arduinobot_msgs.srv import AddTwoInts

class SimpleServiceServer(Node):
    def __init__(self):
        super().__init__('simple_service_server')

        # 서비스 서버 생성 / Create service server
        self.service = self.create_service(
            AddTwoInts,            # 메시지 인터페이스 / Message interface
            'add_two_ints',        # 서비스 이름 / Service name
            self.service_callback  # 콜백 함수 / Callback function
        )
        self.get_logger().info('Service add_two_ints is ready')

    def service_callback(self, request, response):
        # 요청 수신 로그 / Log received request
        self.get_logger().info(
            f'New request received: a={request.a}, b={request.b}'
        )

        # 핵심 로직: 두 정수의 합 계산 / Core logic: calculate sum
        response.sum = request.a + request.b

        self.get_logger().info(f'Returning sum: {response.sum}')
        return response  # 응답 반환 / Return response to client

def main():
    rclpy.init()
    node = SimpleServiceServer()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

### 핵심 API / Key API

| 함수 / Function | 설명 / Description |
|---|---|
| `create_service(type, name, callback)` | 서비스 서버 생성 / Create service server |
| `service_callback(request, response)` | 요청 수신 시 자동 실행 / Auto-called on request |
| `request.a`, `request.b` | 클라이언트가 보낸 데이터 / Data sent by client |
| `response.sum` | 클라이언트에게 반환할 결과 / Result to return |

---

## 서비스 클라이언트 구현 / Service Client Implementation

`arduinobot_examples/simple_service_client.py`

```python
import rclpy
from rclpy.node import Node
from arduinobot_msgs.srv import AddTwoInts
import sys

class SimpleServiceClient(Node):
    def __init__(self, a, b):
        super().__init__('simple_service_client')

        # 클라이언트 생성 / Create client
        self.client = self.create_client(AddTwoInts, 'add_two_ints')

        # 서버 준비될 때까지 1초마다 재시도 / Retry every 1s until server ready
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Service not available, waiting more...')

        # 요청 메시지 생성 / Create request message
        request = AddTwoInts.Request()
        request.a = int(a)  # str → int 형변환 필수 / Type cast required
        request.b = int(b)

        # 비동기 요청 전송 / Send request asynchronously
        future = self.client.call_async(request)
        future.add_done_callback(self.response_callback)

    def response_callback(self, future):
        # 응답 수신 후 결과 출력 / Print result after response received
        self.get_logger().info(f'Service response: {future.result().sum}')

def main():
    rclpy.init()

    # 인자 개수 확인 / Check argument count (script name + a + b = 3)
    if len(sys.argv) != 3:
        print('Wrong number of arguments!')
        print('Usage: simple_service_client a b')
        return -1

    node = SimpleServiceClient(sys.argv[1], sys.argv[2])
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

### 핵심 API / Key API

| 함수 / Function | 설명 / Description |
|---|---|
| `create_client(type, name)` | 서비스 클라이언트 생성 / Create service client |
| `wait_for_service(timeout_sec)` | 서버 준비 대기 / Wait for server to be ready |
| `call_async(request)` | 비동기 요청 전송 / Send request asynchronously |
| `future.add_done_callback(fn)` | 응답 완료 시 실행할 함수 등록 / Register callback for response |

> ⚠️ `sys.argv`로 받은 인자는 기본적으로 **문자열(str)** 타입입니다.
> Arguments from `sys.argv` are **strings** by default — always cast to `int()`.

---

## 빌드 및 실행 / Build & Run

### setup.py 등록 / Register in setup.py

```python
entry_points={
    'console_scripts': [
        'simple_service_server = arduinobot_examples.simple_service_server:main',
        'simple_service_client = arduinobot_examples.simple_service_client:main',
    ],
},
```

### package.xml 의존성 추가 / Add dependency

```xml
<exec_depend>arduinobot_msgs</exec_depend>
```

### 빌드 / Build

```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
```

### 실행 / Run

```bash
# 터미널 1 / Terminal 1: 서비스 서버 실행 / Run service server
ros2 run arduinobot_examples simple_service_server

# 터미널 2 / Terminal 2: 서비스 클라이언트 실행 (5 + 3) / Run client
ros2 run arduinobot_examples simple_service_client 5 3
```

### 실행 결과 / Expected Output

```
# 서버 터미널 / Server terminal
[INFO] Service add_two_ints is ready
[INFO] New request received: a=5, b=3
[INFO] Returning sum: 8

# 클라이언트 터미널 / Client terminal
[INFO] Service response: 8
```

---

## CLI 명령어 / CLI Commands

```bash
# 실행 중인 서비스 목록 / List running services
ros2 service list

# 서비스 인터페이스 타입 확인 / Check service interface type
ros2 service type /add_two_ints

# 터미널에서 직접 요청 / Send request directly from terminal (7 + 5)
ros2 service call /add_two_ints arduinobot_msgs/srv/AddTwoInts "{a: 7, b: 5}"
```

### 시나리오별 동작 / Behavior by Scenario

| 상황 / Situation | 결과 / Result |
|---|---|
| 서버 ON + 클라이언트 `5 3` 실행 / Server ON + client `5 3` | ✅ `Service response: 8` |
| 인자 없이 실행 / Run without arguments | ❌ `Wrong number of arguments!` |
| 서버 OFF + 클라이언트 실행 / Server OFF + client run | ⏳ `Service not available, waiting more...` 반복 / Repeats |
| 대기 중 서버 ON / Server starts while client waits | ✅ 자동 요청 및 응답 수신 / Auto request & response |

---

## 서버 vs 클라이언트 비교 / Server vs Client Summary

| 항목 / Item | 서비스 서버 / Service Server | 서비스 클라이언트 / Service Client |
|---|---|---|
| 생성 함수 / Create fn | `create_service()` | `create_client()` |
| 역할 / Role | 요청 수신 → 처리 → 응답 / Receive → Process → Respond | 요청 전송 → 응답 수신 / Send → Receive |
| 콜백 인자 / Callback args | `(request, response)` | `(future)` |
| 동작 방식 / Behavior | 수동적 — 요청 대기 / Passive — waits for request | 능동적 — 먼저 요청 / Active — initiates request |
