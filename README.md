# ROS2 액션 서버 & 클라이언트 실습 / Action Server & Client Practice

---

## 📌 목차 / Table of Contents

1. [실습 개요 / Practice Overview](#1-실습-개요--practice-overview)
2. [액션 인터페이스 정의 / Define Action Interface](#2-액션-인터페이스-정의--define-action-interface)
3. [액션 서버 구현 / Action Server Implementation](#3-액션-서버-구현--action-server-implementation)
4. [액션 클라이언트 구현 / Action Client Implementation](#4-액션-클라이언트-구현--action-client-implementation)
5. [빌드 및 실행 / Build & Run](#5-빌드-및-실행--build--run)
6. [CLI 명령어 / CLI Commands](#6-cli-명령어--cli-commands)
7. [서버 vs 클라이언트 비교 / Server vs Client Summary](#7-서버-vs-클라이언트-비교--server-vs-client-summary)

---

## 1. 실습 개요 / Practice Overview

> **한국어**
> 피보나치 수열을 계산하는 액션 서버와 이를 요청하는 액션 클라이언트를 Python으로 구현합니다.
> 액션은 시간이 오래 걸리는 작업에 적합한 ROS2 통신 프로토콜로,
> Goal → Feedback → Result 의 흐름으로 동작합니다.

> **English**
> We implement a Fibonacci action server and an action client in Python.
> Actions are a ROS2 communication protocol suited for long-running tasks,
> operating in a Goal → Feedback → Result flow.

### 피보나치 수열이란? / What is the Fibonacci Sequence?

> 각 숫자가 앞의 두 숫자의 합인 수열 (첫 두 숫자는 0과 1로 정의)
> A sequence where each number is the sum of the two preceding ones (first two numbers are 0 and 1)

| Order | 수열 / Sequence |
|---|---|
| 1 | 0, 1 |
| 2 | 0, 1, 1 |
| 3 | 0, 1, 1, 2 |
| 4 | 0, 1, 1, 2, 3 |
| 10 | 0, 1, 1, 2, 3, 5, 8, 13, 21, 34 |

### 메시지 역할 / Message Roles

| 메시지 / Message | 내용 / Content |
|---|---|
| **Goal** | 계산할 수열의 Order / Order of sequence to calculate |
| **Feedback** | 매 단계마다 현재까지의 partial_sequence / Partial sequence at each step |
| **Result** | 완성된 전체 sequence / Complete final sequence |

---

## 2. 액션 인터페이스 정의 / Define Action Interface

> 📌 `arduinobot_msgs` 패키지의 `action/` 폴더에 추가합니다.
> Add to the `action/` folder of the `arduinobot_msgs` package.

**`arduinobot_msgs/action/Fibonacci.action`**

```
# Goal: 계산할 수열의 order / Order of the sequence to calculate
int32 order
---
# Result: 완성된 전체 수열 / Complete Fibonacci sequence
int32[] sequence
---
# Feedback: 현재까지 계산된 부분 수열 / Partial sequence calculated so far
int32[] partial_sequence
```

> ⚠️ `.srv`와 달리 `---`가 **세 번** 사용됩니다 / Unlike `.srv`, `---` is used **three times**:
> - 첫 번째 `---` 위 / Above first `---`: **Goal**
> - 두 `---` 사이 / Between two `---`: **Result**
> - 두 번째 `---` 아래 / Below second `---`: **Feedback**

### CMakeLists.txt (arduinobot_msgs) 업데이트 / Update

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/AddTwoInts.srv"
  "srv/EulerToQuaternion.srv"
  "srv/QuaternionToEuler.srv"
  "action/Fibonacci.action"   # 추가 / Add
)
```

### package.xml (arduinobot_msgs) 업데이트 / Update

```xml
<depend>action_msgs</depend>
```

---

## 3. 액션 서버 구현 / Action Server Implementation

**`arduinobot_examples/simple_action_server.py`**

```python
import rclpy                              # ROS2 Python 클라이언트 라이브러리 / ROS2 Python client library
from rclpy.node import Node               # ROS2 노드 기본 클래스 / Base class for ROS2 nodes
from rclpy.action import ActionServer     # 액션 서버 클래스 / Action server class
from arduinobot_msgs.action import Fibonacci  # 피보나치 액션 인터페이스 / Fibonacci action interface
import time                               # 지연 처리 / For sleep

class SimpleActionServer(Node):
    def __init__(self):
        # 노드 초기화 / Initialize node
        super().__init__('simple_action_server')

        # 액션 서버 생성 / Create action server
        # (노드, 인터페이스, 서버이름, 콜백함수 / node, interface, server name, callback)
        self.action_server = ActionServer(
            self, Fibonacci, 'fibonacci', self.goalCallback
        )
        self.get_logger().info('Starting the server')

    def goalCallback(self, goal_handle):
        # Goal 수신 시 자동 실행 / Auto-called when goal is received
        self.get_logger().info(
            'Received goal request with order %d' % goal_handle.request.order
        )

        # Feedback 메시지 초기화 (첫 두 값: 0, 1)
        # Initialize feedback with first two Fibonacci numbers (0, 1)
        feedback_msg = Fibonacci.Feedback()
        feedback_msg.partial_sequence = [0, 1]

        # 피보나치 수열 계산 루프 / Calculate Fibonacci sequence
        for i in range(1, goal_handle.request.order):

            # 새 원소 = 마지막 + 이전 원소 / New element = last + previous
            feedback_msg.partial_sequence.append(
                feedback_msg.partial_sequence[i] +
                feedback_msg.partial_sequence[i - 1]
            )

            self.get_logger().info(
                'Feedback: {0}'.format(feedback_msg.partial_sequence)
            )

            # 클라이언트에 Feedback 전송 / Send feedback to client
            goal_handle.publish_feedback(feedback_msg)

            # 1초 대기 (오래 걸리는 작업 시뮬레이션) / Wait 1s (simulate long task)
            time.sleep(1)

        # 액션 성공 처리 / Mark goal as succeeded
        goal_handle.succeed()

        # Result 메시지 생성 및 반환 / Create and return result
        result = Fibonacci.Result()
        result.sequence = feedback_msg.partial_sequence
        return result

def main():
    rclpy.init()
    simple_action_server = SimpleActionServer()
    rclpy.spin(simple_action_server)       # Goal 수신 대기 / Wait for goals
    simple_action_server.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
```

### 핵심 API / Key API

| 함수 / Function | 설명 / Description |
|---|---|
| `ActionServer(node, interface, name, callback)` | 액션 서버 생성 / Create action server |
| `goal_handle.request.order` | 클라이언트가 보낸 Goal 데이터 접근 / Access goal data from client |
| `goal_handle.publish_feedback(msg)` | 클라이언트에 Feedback 전송 / Send feedback to client |
| `goal_handle.succeed()` | 액션 성공으로 완료 처리 / Mark action as succeeded |
| `time.sleep(1)` | 오래 걸리는 작업 시뮬레이션 / Simulate long-running task |

---

## 4. 액션 클라이언트 구현 / Action Client Implementation

**`arduinobot_examples/simple_action_client.py`**

```python
import rclpy                              # ROS2 Python 클라이언트 라이브러리 / ROS2 Python client library
from rclpy.node import Node               # ROS2 노드 기본 클래스 / Base class for ROS2 nodes
from rclpy.action import ActionClient     # 액션 클라이언트 클래스 / Action client class
from arduinobot_msgs.action import Fibonacci  # 피보나치 액션 인터페이스 / Fibonacci action interface

class SimpleActionClient(Node):
    def __init__(self):
        # 노드 초기화 / Initialize node
        super().__init__('simple_action_client')

        # 액션 클라이언트 생성 / Create action client
        # (노드, 인터페이스, 서버이름 / node, interface, server name)
        self.action_client = ActionClient(self, Fibonacci, 'fibonacci')

        # 서버 준비될 때까지 블로킹 대기 / Block until server is available
        self.action_client.wait_for_server()

        # Goal 메시지 생성 및 order 설정 / Create goal and set order
        self.goal = Fibonacci.Goal()
        self.goal.order = 10

        # 비동기 Goal 전송 + Feedback 콜백 등록
        # Send goal asynchronously + register feedback callback
        self.future = self.action_client.send_goal_async(
            self.goal,
            feedback_callback=self.feedbackCallback
        )

        # Goal 수락/거절 응답 콜백 등록 / Register response callback
        self.future.add_done_callback(self.responseCallback)

    def responseCallback(self, future):
        # Goal 수락/거절 여부 확인 / Check if goal was accepted or rejected
        goal_handle = future.result()

        if not goal_handle.accepted:
            self.get_logger().info('Goal Rejected')
            return

        self.get_logger().info('Goal Accepted')

        # 비동기 결과 요청 + 결과 콜백 등록
        # Request result asynchronously + register result callback
        self.future = goal_handle.get_result_async()
        self.future.add_done_callback(self.resultCallback)

    def resultCallback(self, future):
        # 최종 결과 수신 / Receive final result
        result = future.result().result
        self.get_logger().info('Result: {0}'.format(result.sequence))

        # 결과 수신 후 ROS2 종료 / Shutdown ROS2 after receiving result
        rclpy.shutdown()

    def feedbackCallback(self, feedback_msg):
        # 주기적 Feedback 수신 시 실행 / Called each time feedback is received
        self.get_logger().info(
            'Received Feedback: {0}'.format(
                feedback_msg.feedback.partial_sequence
            )
        )

def main():
    rclpy.init()
    action_client = SimpleActionClient()
    rclpy.spin(action_client)   # 콜백 처리를 위해 실행 유지 / Keep alive for callbacks

if __name__ == '__main__':
    main()
```

### 핵심 API / Key API

| 함수 / Function | 설명 / Description |
|---|---|
| `ActionClient(node, interface, name)` | 액션 클라이언트 생성 / Create action client |
| `wait_for_server()` | 서버 준비 대기 (블로킹) / Wait for server (blocking) |
| `send_goal_async(goal, feedback_callback)` | 비동기 Goal 전송 / Send goal asynchronously |
| `goal_handle.accepted` | Goal 수락 여부 확인 / Check if goal was accepted |
| `goal_handle.get_result_async()` | 비동기 결과 요청 / Request result asynchronously |
| `future.add_done_callback(fn)` | 완료 시 실행할 함수 등록 / Register callback on completion |

### 세 가지 콜백 함수 / Three Callback Functions

| 콜백 / Callback | 실행 시점 / When Called | 역할 / Role |
|---|---|---|
| `responseCallback` | Goal 전송 직후 / Right after sending goal | 서버의 수락/거절 여부 확인 / Check accepted or rejected |
| `resultCallback` | 액션 완료 후 / After action completes | 최종 결과(전체 수열) 수신 / Receive final result |
| `feedbackCallback` | 서버 실행 중 주기적 / Periodically during execution | 진행 상황(부분 수열) 수신 / Receive partial progress |

---

## 5. 빌드 및 실행 / Build & Run

### setup.py 등록 / Register in setup.py

```python
entry_points={
    'console_scripts': [
        'simple_action_server = arduinobot_examples.simple_action_server:main',
        'simple_action_client = arduinobot_examples.simple_action_client:main',
    ],
},
```

### 빌드 / Build

```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
```

### 실행 / Run

```bash
# 터미널 1 / Terminal 1: 액션 서버 실행 / Run action server
ros2 run arduinobot_examples simple_action_server

# 터미널 2 / Terminal 2: 액션 클라이언트 실행 / Run action client
ros2 run arduinobot_examples simple_action_client
```

### 실행 결과 / Expected Output

```
# 서버 터미널 / Server terminal
[INFO] Starting the server
[INFO] Received goal request with order 10
[INFO] Feedback: [0, 1, 1]
[INFO] Feedback: [0, 1, 1, 2]
[INFO] Feedback: [0, 1, 1, 2, 3]
...

# 클라이언트 터미널 / Client terminal
[INFO] Goal Accepted
[INFO] Received Feedback: [0, 1, 1]
[INFO] Received Feedback: [0, 1, 1, 2]
...
[INFO] Result: [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]
```

### 시나리오별 동작 / Behavior by Scenario

| 상황 / Situation | 결과 / Result |
|---|---|
| 서버 ON + 클라이언트 실행 / Server ON + client run | ✅ Goal 수락 → Feedback → Result |
| 서버 OFF + 클라이언트 실행 / Server OFF + client run | ⏳ `wait_for_server()`에서 블로킹 대기 / Blocked at `wait_for_server()` |
| 대기 중 서버 ON / Server starts while client waits | ✅ 자동으로 Goal 전송 / Auto sends goal |

---

## 6. CLI 명령어 / CLI Commands

```bash
# 실행 중인 액션 목록 / List running actions
ros2 action list

# 액션 상세 정보 확인 / Check action details
ros2 action info /fibonacci -t

# 터미널에서 직접 Goal 전송 (order=10, 피드백 포함)
# Send goal directly from terminal (order=10, with feedback)
ros2 action send_goal /fibonacci \
  arduinobot_msgs/action/Fibonacci \
  "{order: 10}" \
  --feedback
```

---

## 7. 서버 vs 클라이언트 비교 / Server vs Client Summary

| 항목 / Item | 액션 서버 / Action Server | 액션 클라이언트 / Action Client |
|---|---|---|
| 생성 클래스 / Class | `ActionServer` | `ActionClient` |
| 대기 방식 / Wait | Goal 수신 대기 / Waits for goals | `wait_for_server()` |
| Goal 처리 / Goal | `goalCallback()` | `send_goal_async()` |
| Feedback 처리 / Feedback | `publish_feedback()` | `feedbackCallback()` |
| 완료 처리 / Completion | `goal_handle.succeed()` + `return result` | `resultCallback()` |
| 동작 방식 / Behavior | 수동적 / Passive | 능동적 / Active |

---

## 📋 전체 실행 흐름 / Overall Execution Flow

```
SimpleActionClient
        │
        │ wait_for_server() ─── 서버 대기 / Wait for server
        │
        │ send_goal_async(order=10)
        ↓
SimpleActionServer
        │ Goal 수락 / Accept goal
        │   → responseCallback(): "Goal Accepted"
        │
        │ 피보나치 계산 중 (1초마다) / Calculating (every 1s)
        │   → feedbackCallback() 반복 / Repeated
        │     "Received Feedback: [0, 1, 1, 2, ...]"
        │
        │ 계산 완료 / Calculation done
        └── resultCallback()
              "Result: [0,1,1,2,3,5,8,13,21,34]"
              rclpy.shutdown() ── 클라이언트 종료 / Client exits
```
