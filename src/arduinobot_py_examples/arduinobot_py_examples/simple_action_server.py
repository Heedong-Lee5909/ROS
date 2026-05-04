import rclpy                              # ROS2 Python 클라이언트 라이브러리 / ROS2 Python client library
from rclpy.node import Node               # ROS2 노드 기본 클래스 / Base class for ROS2 nodes
from rclpy.action import ActionServer     # 액션 서버 클래스 / Action server class
from arduinobot_msgs.action import Fibonacci  # 피보나치 액션 인터페이스 / Fibonacci action interface
import time                               # 지연(sleep) 처리를 위한 표준 라이브러리 / Standard library for sleep

class SimpleActionServer(Node):
    def __init__(self):
        # 노드 이름을 'simple_action_server'로 초기화
        # Initialize node with name 'simple_action_server'
        super().__init__('simple_action_server')

        # 액션 서버 생성
        # - self          : 현재 노드 / current node
        # - Fibonacci     : 사용할 인터페이스 / interface to use
        # - 'fibonacci'   : 액션 서버 이름 (클라이언트가 이 이름으로 접속) / server name (clients connect by this name)
        # - self.goalCallback: Goal 수신 시 실행할 콜백 함수 / callback called when a goal is received
        self.action_server = ActionServer(
            self,
            Fibonacci,
            'fibonacci',
            self.goalCallback
        )

        # 서버 시작 알림 로그 출력 / Log that the server has started
        self.get_logger().info('Starting the server')

    def goalCallback(self, goal_handle):
        # 클라이언트로부터 Goal 수신 시 자동 실행
        # Automatically called when a goal is received from the client

        # 수신한 Goal의 order 값을 로그로 출력
        # Log the received order value from the goal
        self.get_logger().info(
            'Received goal request with order %d' % goal_handle.request.order
        )

        # Feedback 메시지 객체 생성 / Create feedback message object
        feedback_msg = Fibonacci.Feedback()

        # 피보나치 수열의 첫 두 값(0, 1)으로 partial_sequence 초기화
        # Initialize partial_sequence with the first two Fibonacci numbers (0, 1)
        feedback_msg.partial_sequence = [0, 1]

        # 요청받은 order만큼 피보나치 수열 계산 루프 실행
        # Loop to calculate Fibonacci sequence up to the requested order
        for i in range(1, goal_handle.request.order):

            # 새 원소 = 마지막 원소 + 그 이전 원소
            # New element = last element + second-to-last element
            feedback_msg.partial_sequence.append(
                feedback_msg.partial_sequence[i] +
                feedback_msg.partial_sequence[i - 1]
            )

            # 현재까지 계산된 부분 수열을 터미널에 출력
            # Log the partial sequence calculated so far
            self.get_logger().info(
                'Feedback: {0}'.format(feedback_msg.partial_sequence)
            )

            # 클라이언트에 Feedback 메시지 전송 (진행 상황 알림)
            # Send feedback message to client to report progress
            goal_handle.publish_feedback(feedback_msg)

            # 1초 대기 — 오래 걸리는 작업을 시뮬레이션
            # Wait 1 second — simulates a time-consuming operation
            time.sleep(1)

        # 모든 계산 완료 → 액션을 성공으로 처리
        # All calculations done → mark the goal as succeeded
        goal_handle.succeed()

        # Result 메시지 객체 생성 / Create result message object
        result = Fibonacci.Result()

        # 완성된 전체 피보나치 수열을 결과 메시지에 저장
        # Store the complete Fibonacci sequence in the result message
        result.sequence = feedback_msg.partial_sequence

        # 클라이언트에 Result 메시지 반환 / Return result to the client
        return result

def main():
    # ROS2 초기화 / Initialize ROS2
    rclpy.init()

    # SimpleActionServer 노드 인스턴스 생성
    # Create SimpleActionServer node instance
    simple_action_server = SimpleActionServer()

    # 노드를 계속 실행 상태로 유지 (Goal 수신 대기)
    # Keep node running — waiting for goals from clients
    rclpy.spin(simple_action_server)

    # 노드 종료 처리 / Destroy node on shutdown
    simple_action_server.destroy_node()

    # ROS2 종료 / Shutdown ROS2
    rclpy.shutdown()

if __name__ == '__main__':
    main()
