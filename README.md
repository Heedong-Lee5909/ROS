# Arduinobot Description

## Overview
Added RGB camera link and joint to the existing xacro file, and modified the launch file.
To avoid repetitive build/source/launch cycles when tuning camera position,
launch arguments for rpy and xyz were added so the camera pose can be verified in real time.

## Changes
### 1. RGB Camera (arduinobot.urdf.xacro)
- Added `rgb_camera` link and `rgb_camera_joint`
- Camera fixed to `base_link`

### 2. Launch File (display.launch.py)
- Added `cam_x`, `cam_y`, `cam_z`, `cam_roll`, `cam_pitch`, `cam_yaw` arguments
- Camera pose can be adjusted without rebuild by passing arguments at launch

## Usage
```bash
# Default
ros2 launch arduinobot_description display.launch.py

# With camera pose arguments
ros2 launch arduinobot_description display.launch.py cam_x:=0 cam_y:=0.45 cam_z:=0.2 cam_roll:=-1.57 cam_pitch:=0 cam_yaw:=-1.57
```

---

# 아두이노봇 디스크립션

## 개요
기존 xacro 파일에 카메라 관련 link, joint를 추가하고 launch 파일을 수정했습니다.
카메라 위치 확인 시 반복되는 build/source/launch가 번거로워서
launch 실행 시 rpy, xyz를 인자로 받아 실시간으로 확인할 수 있도록 했습니다.

## 변경사항
### 1. RGB 카메라 (arduinobot.urdf.xacro)
- `rgb_camera` link 및 `rgb_camera_joint` 추가
- `base_link`에 카메라 고정

### 2. Launch 파일 (display.launch.py)
- `cam_x`, `cam_y`, `cam_z`, `cam_roll`, `cam_pitch`, `cam_yaw` 인자 추가
- 빌드 없이 launch 인자만으로 카메라 위치/방향 실시간 확인 가능

## 실행 방법
```bash
# 기본 실행
ros2 launch arduinobot_description display.launch.py

# 카메라 위치/방향 조정
ros2 launch arduinobot_description display.launch.py cam_x:=0 cam_y:=0.45 cam_z:=0.2 cam_roll:=-1.57 cam_pitch:=0 cam_yaw:=-1.57
```
