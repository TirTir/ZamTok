@echo off
REM ZamTok - 5개 창 띄우기 (각각 WSL + 해당 경로)
REM 사용법: cmd에서 wsl 입력 후, 이 스크립트 실행
REM 또는 탐색기에서 더블클릭

start "CLIENT" cmd /k wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/CLIENT && exec bash"
start "SERVER" cmd /k wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/SERVER && exec bash"
start "release-1" cmd /k wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/release && exec bash"
start "release-2" cmd /k wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/release && exec bash"
start "pkg" cmd /k wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/pkg && exec bash"
