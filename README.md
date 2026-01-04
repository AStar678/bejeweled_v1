# Game_v1_release

## 项目简介 / Project Introduction

这是一个宝石消除游戏的项目，包含客户端和服务器端。客户端基于Qt WebEngine实现，服务器使用C++和Crow框架构建，支持PVE和PVP游戏模式。

This is a gem-matching game project, including both client and server components. The client is built with Qt WebEngine, while the server is implemented in C++ using the Crow framework, supporting both PVE and PVP game modes.

## 功能特性 / Features

- **客户端 (Client)**: Qt6 WebEngine应用，提供游戏界面
- **服务器 (Server)**: C++后端，支持游戏逻辑、用户认证、会话管理
- **游戏模式**: 支持PVE（人机对战）和PVP（玩家对战）
- **实时通信**: 基于HTTP的API通信
- **跨平台**: 支持Windows、macOS等平台

- **Client**: Qt6 WebEngine application providing the game interface
- **Server**: C++ backend supporting game logic, user authentication, and session management
- **Game Modes**: Supports PVE (Player vs Environment) and PVP (Player vs Player)
- **Real-time Communication**: HTTP-based API communication
- **Cross-platform**: Supports Windows, macOS, and other platforms

## 技术栈 / Technology Stack

### 客户端 / Client
- Qt6 (Core, Gui, Widgets, WebEngineWidgets)
- C++17
- CMake

### 服务器 / Server
- C++17
- Crow (C++ web framework)
- ASIO (networking library)
- Qt6 (Core, Sql for database operations)
- nlohmann/json (JSON processing)
- CMake

## 安装和运行 / Installation and Running

### 环境要求 / Prerequisites
- CMake 3.16+
- Qt6
- C++17 编译器

### 服务器端 / Server
1. 进入Server目录 / Enter the Server directory:
   ```
   cd Server
   ```

2. 创建构建目录 / Create build directory:
   ```
   mkdir build && cd build
   ```

3. 配置和构建 / Configure and build:
   ```
   cmake ..
   make
   ```

4. 运行服务器 / Run the server:
   ```
   ./GameBackend
   ```
   服务器将在端口8000启动 / The server will start on port 8000.

### 客户端 / Client
1. 进入Client目录 / Enter the Client directory:
   ```
   cd Client
   ```

2. 创建构建目录 / Create build directory:
   ```
   mkdir build && cd build
   ```

3. 配置和构建 / Configure and build:
   ```
   cmake ..
   make
   ```

4. 运行客户端 / Run the client:
   ```
   ./Web
   ```
   客户端将连接到服务器 / The client will connect to the server.

## 使用说明 / Usage

1. 首先启动服务器 / Start the server first.
2. 然后启动客户端 / Then start the client.
3. 在客户端界面中进行游戏 / Play the game through the client interface.

## 项目结构 / Project Structure

```
Game_v1_release/
├── Client/                 # 客户端代码 / Client code
│   ├── CMakeLists.txt
│   └── main.cpp
├── Server/                 # 服务器代码 / Server code
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── lib/               # 第三方库 / Third-party libraries
│   │   ├── asio.hpp
│   │   ├── crow_all.h
│   │   └── json.hpp
│   └── src/               # 源代码 / Source code
│       ├── main.cpp
│       ├── config/
│       ├── controllers/
│       ├── models/
│       ├── services/
│       ├── static/
│       └── utils/
```

## API文档 / API Documentation

详细的API接口文档请参考 / For detailed API documentation, please refer to:
- [CLIENT.md](Server/CLIENT.md) - 前端API接口规范 / Frontend API specification
- [SERVICEDEVELOP.md](Server/SERVICEDEVELOP.md) - 服务端开发需求 / Server development requirements

## 贡献 / Contributing

本项目由本人及两位合作者共同开发。

This project is developed by the author and two collaborators.

### 作者 / Authors
- [@Ao.Xiang.Axel](https://github.com/AStar678)（本人/I） - 架构设计 / Architecture Design
- [@hjy0122](https://github.com/hjy0122) - 前端完成者 / Frontend Developer
- [@sekkeazin333](https://github.com/24301001) - 后端完成者 / Backend Developer

## 致谢 / Acknowledgments

感谢以下开源软件包的提供者为本项目提供了强大的技术支持：

Thanks to the providers of the following open-source packages for providing strong technical support for this project:

- [Qt6](https://www.qt.io/) - 跨平台GUI框架 / Cross-platform GUI framework
- [Crow](https://github.com/CrowCpp/Crow) - C++ web框架 / C++ web framework
- [ASIO](https://think-async.com/Asio/) - 网络编程库 / Networking library
- [nlohmann/json](https://github.com/nlohmann/json) - JSON处理库 / JSON processing library

特别感谢授课老师赵宏老师的指导和支持。

Special thanks to instructor Zhao Hong for his guidance and support.

