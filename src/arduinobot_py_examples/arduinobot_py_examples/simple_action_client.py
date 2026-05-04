import rclpy                              # ROS2 Python 클라이언트 라이브러리 / ROS2 Python client library
from rclpy.node import Node               # ROS2 노드 기본 클래스 / Base class for ROS2 nodes
from rclpy.action import ActionClient     # 액션 클라이언트 클래스 / Action client class
from arduinobot_msgs.action import Fibonacci  # 피보나치 액션 인터페이스 / Fibonacci action interface

class SimpleActionClient(Node):
    def __init__(self):
        # 노드 이름을 'simple_action_client'로 초기화
        # Initialize node with name 'simple_action_client'
        super().__init__('simple_action_client')

        # 액션 클라이언트 생성
        # - self     : 현재 노드 / current node
        # - Fibonacci: 사용할 인터페이스 / interface to use
        # - 'fibonacci': 접속할 액션 서버 이름 / action server name to connect to
        self.action_client = ActionClient(self, Fibonacci, 'fibonacci')

        # 'fibonacci' 액션 서버가 준비될 때까지 블로킹 대기
        # Block execution until the 'fibonacci' action server is available
        self.action_client.wait_for_server()

        # Goal 메시지 객체 생성 / Create goal message object
        self.goal = Fibonacci.Goal()

        # Goal의 order 값을 10으로 설정 (10번째 피보나치 수열까지 계산 요청)
        # Set order to 10 — request Fibonacci sequence up to order 10
        self.goal.order = 10

        # 비동기로 Goal 전송
        # - feedback_callback: Feedback 메시지 수신 시 실행할 함수 등록
        # Send goal asynchronously
        # - feedback_callback: function to call whenever feedback is received
        self.future = self.action_client.send_goal_async(
            self.goal,
            feedback_callback=self.feedbackCallback
        )

        # Goal 전송 후 서버의 수락/거절 응답이 오면 실행할 콜백 등록
        # Register callback to be called when server responds to the goal (accepted/rejected)
        self.future.add_done_callback(self.responseCallback)

    def responseCallback(self, future):
        # future.result()로 goal_handle 획득
        # goal_handle: Goal 수락 여부 확인 및 결과 요청에 사용
        # Retrieve goal_handle — used to check acceptance and request result
        goal_handle = future.result()

        # Goal이 거절된 경우 로그 출력 후 종료
        # If goal was rejected, log message and return
        if not goal_handle.accepted:
            self.get_logger().info('Goal Rejected')
            return

        # Goal이 수락된 경우 로그 출력
        # Log if goal was accepted by the server
        self.get_logger().info('Goal Accepted')

        # 비동기로 최종 결과 요청
        # Request the final result from the server asynchronously
        self.future = goal_handle.get_result_async()

        # 결과 수신 완료 시 실행할 콜백 등록
        # Register callback to be called when result is received
        self.future.add_done_callback(self.resultCallback)

    def resultCallback(self, future):
        # future.result().result로 액션 서버의 최종 결과 메시지 획득
        # Get the final result message from the action server
        result = future.result().result

        # 결과인 전체 피보나치 수열을 터미널에 출력
        # Print the complete Fibonacci sequence received as result
        self.get_logger().info('Result: {0}'.format(result.sequence))

        # 결과 수신 완료 후 ROS2 종료
        # Shutdown ROS2 after receiving the final result
        rclpy.shutdown()

    def feedbackCallback(self, feedback_msg):
        # 서버가 주기적으로 보내는 Feedback 메시지 수신 시 실행
        # Called each time the server sends a periodic feedback message

        # 현재까지 계산된 부분 수열(partial_sequence)을 터미널에 출력
        # Print the partial Fibonacci sequence calculated so far
        self.get_logger().info(
            'Received Feedback: {0}'.format(feedback_msg.feedback.partial_sequence)
        )

def main():
    # ROS2 초기화 / Initialize ROS2
    rclpy.init()

    # SimpleActionClient 노드 인스턴스 생성
    # 생성자 내부에서 Goal 전송까지 자동 실행됨
    # Create SimpleActionClient node — goal is sent automatically in constructor
    action_client = SimpleActionClient()

    # 노드를 계속 실행 상태로 유지 (콜백 처리를 위해)
    # Keep node alive to process incoming callbacks (feedback, result)
    rclpy.spin(action_client)

if __name__ == '__main__':
    main()
