#include <memory>                                                    // shared_ptr 사용을 위한 헤더 / For shared_ptr
#include <rclcpp/rclcpp.hpp>                                         // ROS2 C++ 클라이언트 라이브러리 / ROS2 C++ client library
#include <moveit/move_group_interface/move_group_interface.h>        // MoveIt2 C++ API 헤더 / MoveIt2 C++ API header

// ── move_robot 함수 ──────────────────────────────────────────────────
// 노드를 입력받아 MoveIt2 API로 로봇을 목표 위치로 이동시키는 함수
// Function that receives a node and moves the robot to a target position using MoveIt2 API
void move_robot(const std::shared_ptr<rclcpp::Node> node)
{
    // arm Move Group 접근 (SRDF에서 정의한 'arm' 그룹)
    // Access arm move group (defined as 'arm' in SRDF)
    auto arm_move_group = moveit::planning_interface::MoveGroupInterface(node, "arm");

    // gripper Move Group 접근 (SRDF에서 정의한 'gripper' 그룹)
    // Access gripper move group (defined as 'gripper' in SRDF)
    auto gripper_move_group = moveit::planning_interface::MoveGroupInterface(node, "gripper");

    // arm 목표 관절 위치 설정 (단위: 라디안 / Unit: radians)
    // joint1 = 1.57 rad (90°) → 베이스 90° 회전 / Rotate base 90°
    // joint2 = 0.0, joint3 = 0.0 → 나머지 관절 고정 / Keep other joints fixed
    std::vector<double> arm_joint_goal {1.57, 0.0, 0.0};

    // gripper 목표 관절 위치 설정 (단위: 라디안 / Unit: radians)
    // joint4 = -0.7, joint5 = 0.7 → 반대 방향으로 회전하여 그리퍼 열림
    // joint4 = -0.7, joint5 = 0.7 → Rotate in opposite directions to open gripper
    std::vector<double> gripper_joint_goal {-0.7, 0.7};

    // arm 목표 관절 위치 적용
    // - 반환값(bool): 목표가 관절 범위 내에 있으면 true, 범위 초과 시 false
    // Set arm joint target
    // - Returns true if within limits, false if out of limits
    bool arm_within_bounds = arm_move_group.setJointValueTarget(arm_joint_goal);

    // gripper 목표 관절 위치 적용
    // - 반환값(bool): 목표가 관절 범위 내에 있으면 true, 범위 초과 시 false
    // Set gripper joint target
    // - Returns true if within limits, false if out of limits
    bool gripper_within_bounds = gripper_move_group.setJointValueTarget(gripper_joint_goal);

    // 목표 위치가 관절 범위를 벗어난 경우 경고 출력 후 함수 종료
    // If target positions are out of joint limits, warn and return
    if(!arm_within_bounds || !gripper_within_bounds)
    {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"),
                    "Target joint position(s) were outside of limits, but we will plan and clamp to the limits");
        return;
    }

    // arm 궤적 계획 결과를 저장할 Plan 객체 생성
    // Create Plan object to store arm trajectory planning result
    moveit::planning_interface::MoveGroupInterface::Plan arm_plan;

    // gripper 궤적 계획 결과를 저장할 Plan 객체 생성
    // Create Plan object to store gripper trajectory planning result
    moveit::planning_interface::MoveGroupInterface::Plan gripper_plan;

    // arm 궤적 계획 실행
    // - 현재 위치 → 목표 위치까지의 충돌 없는 경로 자동 계산
    // - 결과가 SUCCESS이면 true 저장
    // Plan arm trajectory
    // - Auto-computes collision-free path from current to target position
    // - Stores true if result is SUCCESS
    bool arm_plan_success = (arm_move_group.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS);

    // gripper 궤적 계획 실행
    // - 결과가 SUCCESS이면 true 저장
    // Plan gripper trajectory
    // - Stores true if result is SUCCESS
    bool gripper_plan_success = (gripper_move_group.plan(gripper_plan) == moveit::core::MoveItErrorCode::SUCCESS);

    // 두 계획이 모두 성공한 경우 궤적 실행
    // If both plans succeeded, execute trajectories
    if(arm_plan_success && gripper_plan_success)
    {
        // 계획된 arm 궤적 실행 → 로봇 팔이 목표 위치로 이동
        // Execute planned arm trajectory → robot arm moves to target
        arm_move_group.move();

        // 계획된 gripper 궤적 실행 → 그리퍼가 목표 위치로 이동
        // Execute planned gripper trajectory → gripper moves to target
        gripper_move_group.move();
    }
    else
    {
        // 계획 실패 시 에러 메시지 출력 후 종료
        // 목표 위치가 유효하지 않거나 충돌로 인해 경로를 찾지 못한 경우 발생
        // Print error and return if planning failed
        // Occurs when target pose is invalid or no collision-free path found
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "One or more planners failed");
        return;
    }
}

// ── main 함수 ────────────────────────────────────────────────────────
int main(int argc, char **argv)
{
    // ROS2 초기화 / Initialize ROS2
    rclcpp::init(argc, argv);

    // 'simple_moveit_interface' 이름의 ROS2 노드 생성
    // Create ROS2 node named 'simple_moveit_interface'
    std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("simple_moveit_interface");

    // 로봇 이동 함수 실행 (목표 설정 → 계획 → 실행)
    // Execute robot movement (set target → plan → execute)
    move_robot(node);

    // 노드를 계속 실행 상태로 유지 (콜백 처리 대기)
    // Keep node running to process callbacks
    rclcpp::spin(node);

    // ROS2 종료 / Shutdown ROS2
    rclcpp::shutdown();
}
