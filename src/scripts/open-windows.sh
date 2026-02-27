#!/bin/bash

cmd.exe /c wt -w 0 \
new-tab wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/CLIENT && exec bash" \; \
new-tab wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/SERVER && exec bash" \; \
new-tab wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/src/release && exec bash" \; \
new-tab wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok/pkg && exec bash" \; \
new-tab wsl -d Ubuntu-24.04 -e bash -c "cd /home/user/dev/ZamTok && exec bash"
