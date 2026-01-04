#pragma once
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include <memory>
#include "json.hpp"
#include "../config/GameConfig.h"

struct GameSession {
    std::string uuid; // 游戏会话的唯一标识符
    int uid = 0; // 用户ID
    std::string nickname = "Player"; // 用户昵称
    std::string mode; // 游戏模式（例如：pvp, pve等）
    int level = 1; // 当前关卡等级
    int current_score = 0; // 当前得分
    int moves_left = -1; // 剩余移动次数
    bool is_over = false; // 游戏是否结束
    bool is_win = false; // 是否获胜
    std::string end_reason; // 游戏结束原因

    // PVP/PVE 字段
    bool is_pvp = false; // 是否为PVP模式
    bool is_ai = false; // 是否为AI对手
    int ai_difficulty = 1; // AI难度等级
    long long last_ai_move_time = 0; // 最后一次AI移动的时间戳
    std::string opponent_uuid = ""; // 对手的UUID
    std::string opponent_nickname = "Opponent"; // 对手昵称
    bool opponent_quit = false;                 // 对手是否强退

    long long start_time = 0;        // 游戏开始时间

    // 冰冻与免疫字段
    long long frozen_until = 0;      // 冻结直到...
    long long immunity_until = 0;    // 免疫直到... (新增)

    std::vector<std::vector<int>> map; // 游戏地图数据
    std::vector<std::vector<bool>> ice_map; // 冰块地图
    std::map<int, int> bomb_map; // 炸弹位置映射
    LevelConfig config; // 关卡配置

    std::vector<nlohmann::json> event_queue; // 事件队列
    std::shared_ptr<std::mutex> event_mutex; // 事件队列的互斥锁

    GameSession() {
        event_mutex = std::make_shared<std::mutex>();
    }

    GameSession(std::string id, int u, std::string nick, std::string m, int l)
        : uuid(id), uid(u), nickname(nick), mode(m), level(l) {

        event_mutex = std::make_shared<std::mutex>();
        map.resize(8, std::vector<int>(8));
        ice_map.resize(8, std::vector<bool>(8, false));

        config = GameConfig::getLevelConfig(l);
        moves_left = config.max_moves;

        if (mode == "pvp" || mode == "pve") {
            is_pvp = true;
        }

        using namespace std::chrono;
        start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        last_ai_move_time = start_time;
    }

    static int posKey(int r, int c) { return r * 8 + c; }
};