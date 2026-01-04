#pragma once
#include <vector>
#include <string>

struct LevelConfig {
    int level_id; // 关卡ID
    int target_score; // 目标得分
    int max_moves; // 最大移动次数
    int ice_count; // 冰块数量
    int bomb_count; // 炸弹数量
    int bomb_initial_time; // 炸弹初始时间
    int virus_count; // 病毒数量
    std::string desc; // 关卡描述
};

class GameConfig {
public:
    inline static const int COIN_REWARD_LEVEL_PASS = 100;  // 通关关卡的金币奖励
    inline static const int COIN_DIVISOR_ENDLESS = 100; // 无尽模式的金币除数

    inline static const int ITEM_COST_BOMB = 30; // 炸弹道具成本
    inline static const int ITEM_COST_RESET = 30; // 重置道具成本
    inline static const int ITEM_COST_FREEZE = 30; // 冻结道具成本

    inline static const int AI_DELAY_EASY = 5000;   // 简单AI延迟（毫秒）
    inline static const int AI_DELAY_NORMAL = 3000; // 普通AI延迟（毫秒）
    inline static const int AI_DELAY_HARD = 1000;    // 困难AI延迟（毫秒）

    inline static const int FREEZE_DURATION_MS = 3000; // 冻结持续时间（毫秒）
    // ------------------------------------

    static LevelConfig getLevelConfig(int level) {
        switch (level) {
            case 1: return {1, 1000, -1, 0, 0, 0, 0, "LV1: 热身运动"};
            case 2: return {2, 1000, -1, 12, 0, 0, 0, "LV2: 极寒冻土"};
            case 3: return {3, 1000, -1, 0, 3, 15, 0, "LV3: 绝命拆弹"};
            case 4: return {4, 1000, -1, 0, 0, 0, 3, "LV4: 病毒危机"};
            case 5: return {5, 1000, 20, 0, 0, 0, 0, "LV5: 步步惊心"};
            default: return {0, 99999, -1, 0, 0, 0, 0, "未知关卡"};
        }
    }
};