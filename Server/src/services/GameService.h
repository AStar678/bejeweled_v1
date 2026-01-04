#pragma once

#include "../models/GameSession.h"
#include "../config/GameConfig.h"
#include "../dao/UserDao.h"

#include <random>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <memory>
#include <string>

// 简单的坐标点结构
struct Point {
    int r, c;
    // 用于 set 排序去重
    bool operator<(const Point& o) const { 
        return r < o.r || (r == o.r && c < o.c); 
    }
};

// AI 算路用的结构
struct Move {
    int r, c;
    std::string dir;
    int score;
};

class GameService {
public:
    // 单例获取
    static GameService& getInstance();

    // 禁止拷贝
    GameService(const GameService&) = delete;
    void operator=(const GameService&) = delete;

    // --- 核心业务 API ---

    nlohmann::json getLeaderboard();
    nlohmann::json startPVE(int uid, int diff);
    nlohmann::json joinPVP(int uid);
    bool cancelMatch(int uid);
    void quitGame(const std::string& uuid);
    nlohmann::json getDualState(const std::string& uuid);
    nlohmann::json processMove(const std::string& uuid, int row, int col, const std::string& direction);
    nlohmann::json buyItem(int uid, const std::string& itemType);
    nlohmann::json useItem(const std::string& uuid, const std::string& itemType, int r = -1, int c = -1);
    GameSession* createSession(int uid, const std::string& mode, int level);
    std::shared_ptr<GameSession> getSession(const std::string& uuid);

private:
    GameService() = default; // 私有构造

    UserDao userDao;

    // 数据存储
    std::map<std::string, std::shared_ptr<GameSession>> sessions;
    std::mutex session_mutex;
    std::string waiting_pvp_uuid = ""; // 正在排队的那个人

    // --- 内部辅助函数 ---

    // 随机工具
    int randomGem();
    int randomInt(int min, int max);
    long long nowMs();

    // 统计场面
    int countIce(const GameSession& s);
    int countBombs(const GameSession& s);
    int countViruses(const GameSession& s);

    // 游戏逻辑核心
    void generateMap(GameSession& session); 
    std::set<Point> findMatches(const std::vector<std::vector<int>>& map); 
    void handleSpecialEliminations(GameSession& s, std::set<Point>& m); 
    void applyElimination(GameSession& s, const std::set<Point>& m); 
    nlohmann::json spreadVirus(GameSession& s); 
    nlohmann::json forceSpawnViruses(GameSession& s, int count); 

    // AI 逻辑
    std::vector<Move> getAllMoves(const std::vector<std::vector<int>>& map);
    void updateAI(GameSession& ai);
};