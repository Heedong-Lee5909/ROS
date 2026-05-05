#include <rclcpp/rclcpp.hpp>                                          // ROS2 C++ 클라이언트 라이브러리 / ROS2 C++ client library
#include <rclcpp_action/rclcpp_action.hpp>                            // ROS2 액션 서버/클라이언트 / ROS2 action server/client
#include <rclcpp_components/register_node_macro.hpp>                  // 노드 컴포넌트 등록 매크로 / Node component registration macro
#include <moveit/move_group_interface/move_group_interface.h>         // MoveIt2 C++ API / MoveIt2 C++ API
#include "arduinobot_msgs/action/arduinobot_task.hpp"                 // 커스텀 액션 인터페이스 / Custom action interface

#include <memory>   // shared_ptr 사용 / For shared_ptr
#include <thread>   // 별도 스레드 실행 / For separate thread execution

using namespace std::placeholders;  // _1, _2 플레이스홀더 사용 / For _1, _2 placeholders

namespace arduinobot_remote
{
class TaskServer : public rclcpp::Node
{
public:
    explicit TaskServer(const rclcpp::NodeOptions &options = rclcpp::NodeOptions())
        : Node("task_server", options)  // 노드 이름 'task_server'로 초기화 / Initialize node with name 'task_server'
    {
        RCLCPP_INFO(get_logger(), "Starting the Server");

        // 액션 서버 생성
        // - this: 현재 노드 / current node
        // - "task_server": 액션 서버 이름 (클라이언트가 이 이름으로 접속) / server name
        // - goalCallback: Goal 수신 시 실행 / called when goal received
        // - cancelCallback: 취소 요청 시 실행 / called when cancel requested
        // - acceptedCallback: Goal 수락 후 실행 / called after goal accepted
        action_server_ = rclcpp_action::create_server<arduinobot_msgs::action::ArduinobotTask>(
            this, "task_server",
            std::bind(&TaskServer::goalCallback,     this, _1, _2),
            std::bind(&TaskServer::cancelCallback,   this, _1),
            std::bind(&TaskServer::acceptedCallback, this, _1)
        );
    }

private:
    // 액션 서버 객체 / Action server object
    rclcpp_action::Server<arduinobot_msgs::action::ArduinobotTask>::SharedPtr action_server_;

    // MoveIt2 Move Group 포인터 (처음 execute 호출 시 초기화)
    // MoveIt2 Move Group pointers (initialized on first execute call)
    std::shared_ptr<moveit::planning_interface::MoveGroupInterface> arm_move_group_, gripper_move_group_;

    // 목표 관절 위치 벡터 / Target joint position vectors
    std::vector<double> arm_joint_goal_, gripper_joint_goal_;

    // ── Goal 수신 콜백 / Goal received callback ──────────────────────────
    // 클라이언트로부터 Goal이 도착했을 때 자동 실행
    // Automatically called when a goal arrives from the client
    rclcpp_action::GoalResponse goalCallback(
        const rclcpp_action::GoalUUID &uuid,
        std::shared_ptr<const arduinobot_msgs::action::ArduinobotTask::Goal> goal)
    {
        // 수신한 태스크 번호 로그 출력 / Log received task number
        RCLCPP_INFO(get_logger(), "Received goal request with task number: %d", goal->task_number);
        (void)uuid;
        // 모든 Goal을 수락하고 즉시 실행 / Accept and execute all goals
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    }

    // ── 취소 요청 콜백 / Cancel request callback ─────────────────────────
    // 클라이언트가 실행 중인 Goal을 취소 요청할 때 실행
    // Called when client requests cancellation of an ongoing goal
    rclcpp_action::CancelResponse cancelCallback(
        const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        RCLCPP_INFO(get_logger(), "Received request to cancel goal");

        // 현재 이동 중인 arm 정지 / Stop arm if moving
        if(arm_move_group_)     arm_move_group_->stop();
        // 현재 이동 중인 gripper 정지 / Stop gripper if moving
        if(gripper_move_group_) gripper_move_group_->stop();

        (void)goal_handle;
        return rclcpp_action::CancelResponse::ACCEPT;  // 취소 수락 / Accept cancellation
    }

    // ── Goal 수락 후 실행 콜백 / Accepted callback ───────────────────────
    // goalCallback에서 수락된 직후 실행되며, execute를 별도 스레드로 시작
    // Called right after goal is accepted, starts execute in a separate thread
    void acceptedCallback(
        const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        // 별도 스레드로 execute 실행 (메인 스레드 블로킹 방지)
        // Run execute in separate thread (prevents blocking main thread)
        std::thread{std::bind(&TaskServer::execute, this, _1), goal_handle}.detach();
    }

    // ── 핵심 실행 함수 / Core execute function ───────────────────────────
    // 실제 로봇 이동 로직이 담긴 함수
    // Contains the actual robot motion logic
    void execute(const std::shared_ptr<rclcpp_action::ServerGoalHandle<arduinobot_msgs::action::ArduinobotTask>> goal_handle)
    {
        RCLCPP_INFO(get_logger(), "Executing goal");

        // Move Group이 아직 초기화되지 않은 경우에만 생성
        // Create move groups only if not yet initialized
        if(!arm_move_group_)
            arm_move_group_ = std::make_shared<moveit::planning_interface::MoveGroupInterface>(
                shared_from_this(), "arm");      // SRDF의 'arm' 그룹에 접근 / Access 'arm' group in SRDF

        if(!gripper_move_group_)
            gripper_move_group_ = std::make_shared<moveit::planning_interface::MoveGroupInterface>(
                shared_from_this(), "gripper");  // SRDF의 'gripper' 그룹에 접근 / Access 'gripper' group in SRDF

        // 클라이언트에 반환할 Result 메시지 생성 / Create result message to return to client
        auto result = std::make_shared<arduinobot_msgs::action::ArduinobotTask::Result>();

        // ── 태스크 ID별 목표 관절 위치 설정 / Set joint targets by task ID ──
        if(goal_handle->get_goal()->task_number == 0)
        {
            // Task 0: 홈 포지션 + 그리퍼 열림
            // joint1=0, joint2=0, joint3=0 / gripper open(-0.7, 0.7)
            arm_joint_goal_     = {0.0, 0.0, 0.0};
            gripper_joint_goal_ = {-0.7, 0.7};
        }
        else if(goal_handle->get_goal()->task_number == 1)
        {
            // Task 1: 픽 포지션 + 그리퍼 닫기
            // 사전 계산된 파지 위치 / Pre-calculated pick position
            arm_joint_goal_     = {-1.14, -0.6, -0.07};
            gripper_joint_goal_ = {0.0, 0.0};  // 그리퍼 닫힘 / Gripper closed
        }
        else if(goal_handle->get_goal()->task_number == 2)
        {
            // Task 2: 레스트 포지션 (팔이 접힌 상태)
            // Rest position (arm folded)
            arm_joint_goal_     = {-1.57, 0.0, -0.9};
            gripper_joint_goal_ = {0.0, 0.0};  // 그리퍼 닫힘 / Gripper closed
        }
        else
        {
            // 유효하지 않은 태스크 번호 → 에러 출력 후 종료
            // Invalid task number → log error and return
            RCLCPP_ERROR(get_logger(), "Invalid task number");
            return;
        }

        // 시작 상태를 현재 상태로 설정 (플래너가 현재 위치를 출발점으로 사용)
        // Set start state to current state (planner uses current position as start)
        arm_move_group_->setStartState(*arm_move_group_->getCurrentState());
        gripper_move_group_->setStartState(*gripper_move_group_->getCurrentState());

        // 목표 관절 위치 설정
        // - 반환값(bool): 목표가 관절 범위 내 → true / 범위 초과 → false
        // Set target joint positions
        // - Returns true if within limits, false if out of limits
        bool arm_within_bounds     = arm_move_group_->setJointValueTarget(arm_joint_goal_);
        bool gripper_within_bounds = gripper_move_group_->setJointValueTarget(gripper_joint_goal_);

        // 목표가 범위를 벗어난 경우 에러 출력 후 종료
        // Log error and return if target is out of joint limits
        if(!arm_within_bounds || !gripper_within_bounds)
        {
            RCLCPP_ERROR(get_logger(), "Target position out of boundaries");
            return;
        }

        // 궤적 계획 결과를 저장할 Plan 객체 생성
        // Create Plan objects to store trajectory planning results
        moveit::planning_interface::MoveGroupInterface::Plan arm_plan, gripper_plan;

        // arm 궤적 계획 실행 + 성공 여부 확인
        // Plan arm trajectory + check if succeeded
        bool arm_plan_success =
            (arm_move_group_->plan(arm_plan) ==
             moveit::core::MoveItErrorCode::SUCCESS);

        // gripper 궤적 계획 실행 + 성공 여부 확인
        // Plan gripper trajectory + check if succeeded
        bool gripper_plan_success =
            (gripper_move_group_->plan(gripper_plan) ==
             moveit::core::MoveItErrorCode::SUCCESS);

        // 두 계획 모두 성공 시 실행 / Execute if both plans succeeded
        if(arm_plan_success && gripper_plan_success)
        {
            arm_move_group_->move();      // arm 궤적 실행 → 로봇 팔 이동 / Execute arm trajectory
            gripper_move_group_->move();  // gripper 궤적 실행 → 그리퍼 이동 / Execute gripper trajectory
        }
        else
        {
            // 계획 실패 시 에러 출력 후 종료
            // Log error and return if planning failed
            RCLCPP_ERROR(get_logger(), "One or more planners failed");
            return;
        }

        // 액션 성공 처리 + Result 메시지 반환
        // Mark action as succeeded + return result to client
        result->success = true;
        goal_handle->succeed(result);
        RCLCPP_INFO(get_logger(), "Goal succeeded");
    }
};
} // namespace arduinobot_remote

// 이 노드를 ROS2 컴포넌트로 등록 (런치 파일에서 로드 가능하도록)
// Register this node as a ROS2 component (allows loading from launch file)
RCLCPP_COMPONENTS_REGISTER_NODE(arduinobot_remote::TaskServer)
