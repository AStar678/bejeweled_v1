#pragma once
#include <string>
#include "json.hpp"

// 用户数据模型
struct User {
    int uid; // 用户唯一ID
    std::string nickname; // 用户昵称
    std::string account; // 用户账号
    std::string password; // 用户密码（实际生产中应存储哈希值）
    int coins = 500;      // 初始金币 [cite: 320]
    int max_score = 0; // 最高得分
    int max_level = 1; // 最高通关等级
    int item_bomb = 0; // 炸弹道具数量
    int item_reset = 0; // 重置道具数量
    int item_freeze = 0; // 冻结道具数量

    nlohmann::json toAssetsJson() const {
        return {
                {"coins", coins},
                {"max_score", max_score},
                {"max_level", max_level},
                {"items", {
                    {"bomb", item_bomb},
                    {"reset", item_reset},
                    {"freeze", item_freeze}
                }}
        };
    }
};