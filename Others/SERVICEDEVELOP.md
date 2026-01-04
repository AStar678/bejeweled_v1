# GameService 开发需求文档

## 概述
GameService 是游戏服务器的核心业务逻辑类，负责管理游戏会话、处理游戏逻辑、数据库交互等。作为单例模式实现，需要实现以下功能模块。

## 依赖
- `GameSession` 模型 (src/models/GameSession.h)
- `User` 模型 (src/models/User.h)
- `GameConfig` 配置类 (src/config/GameConfig.h)
- Qt SQL 数据库操作
- nlohmann::json 用于数据序列化

## 核心接口需求

### 1. 单例模式
```cpp
class GameService {
public:
    static GameService& getInstance();
private:
    GameService(); // 私有构造函数
};
```

### 2. 会话管理
```cpp
// 创建游戏会话
GameSession* createSession(int uid, const std::string& mode, int level);

// 获取会话 (返回 shared_ptr)
std::shared_ptr<GameSession> getSession(const std::string& uuid);

// 退出游戏
void quitGame(const std::string& uuid);
```

### 3. 游戏逻辑处理
```cpp
// 处理移动操作
nlohmann::json processMove(const std::string& uuid, int row, int col, const std::string& direction);

// 开始PVE模式
nlohmann::json startPVE(int uid, int difficulty);

// PVP匹配
nlohmann::json joinPVP(int uid);

// 取消PVP匹配
bool cancelMatch(int uid);

// 获取对战状态 (PVP/PVE)
nlohmann::json getDualState(const std::string& uuid);
```

### 4. 道具系统
```cpp
// 购买道具
nlohmann::json buyItem(int uid, const std::string& itemType);

// 使用道具
nlohmann::json useItem(const std::string& uuid, const std::string& itemType, int row = -1, int col = -1);
```

### 5. 排行榜
```cpp
// 获取排行榜
nlohmann::json getLeaderboard();
```

## 数据模型接口

### User 模型
```cpp
struct User {
    int uid;
    std::string nickname;
    std::string account;
    std::string password;
    int coins = 500;
    int max_score = 0;
    int max_level = 1;

    nlohmann::json toAssetsJson() const {
        return {
            {"coins", coins},
            {"max_score", max_score},
            {"max_level", max_level}
        };
    }
};
```

### GameSession 模型
```cpp
struct GameSession {
    std::string uuid;
    int uid = 0;
    std::string nickname = "Player";
    std::string mode;
    int level = 1;
    int current_score = 0;
    int moves_left = -1;
    bool is_over = false;
    bool is_win = false;
    std::string end_reason;

    // PVP/PVE 相关
    bool is_pvp = false;
    bool is_ai = false;
    int ai_difficulty = 1;
    long long last_ai_move_time = 0;
    std::string opponent_uuid = "";
    std::string opponent_nickname = "Opponent";
    bool opponent_quit = false;

    long long start_time = 0;
    long long frozen_until = 0;
    long long immunity_until = 0;

    std::vector<std::vector<int>> map;        // 8x8 游戏地图
    std::vector<std::vector<bool>> ice_map;  // 8x8 冰块地图
    std::map<int, int> bomb_map;             // 炸弹位置和倒计时

    LevelConfig config;
    std::vector<nlohmann::json> event_queue;
    std::shared_ptr<std::mutex> event_mutex;

    // 构造函数
    GameSession(std::string id, int u, std::string nick, std::string m, int l);
    static int posKey(int r, int c) { return r * 8 + c; }
};
```

## 数据库操作需求

### 用户数据表结构
```sql
CREATE TABLE users (
    uid INTEGER PRIMARY KEY AUTOINCREMENT,
    account TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    nickname TEXT DEFAULT 'Player',
    coins INTEGER DEFAULT 500,
    item_bomb INTEGER DEFAULT 0,
    item_reset INTEGER DEFAULT 0,
    item_freeze INTEGER DEFAULT 0,
    max_score INTEGER DEFAULT 0,
    max_level INTEGER DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 需要的数据库辅助方法
```cpp
// 从数据库获取用户昵称
std::string getNicknameFromDB(int uid);

// 更新用户资产 (金币、道具、分数等)
bool updateAsset(int uid, const std::string& field, int delta);

// 更新最高分数
bool updateMaxScore(int uid, int score);

// 更新最高关卡
bool updateMaxLevel(int uid, int level);
```

## 游戏逻辑需求

### 地图生成
- 生成8x8的宝石地图 (1-5表示不同颜色宝石)
- 根据关卡配置添加冰块、炸弹、病毒等特殊元素

### 匹配检测
- 检测横向/纵向3个或以上相同宝石的匹配
- 返回匹配的坐标点集合

### 消除处理
- 执行宝石消除
- 处理特殊消除效果 (炸弹连锁等)
- 应用重力填充新宝石

### AI逻辑 (PVE模式)
- 根据难度实现不同的AI移动策略
- 模拟玩家操作进行移动

### 道具效果
- **炸弹(bomb)**: 消除3x3区域内的所有宝石
- **重置(reset)**: 重新生成整个地图
- **冻结(freeze)**: 冻结对手3秒

### 事件系统
- 记录游戏事件 (交换、消除、填充等)
- 支持PVP模式下的事件同步给对手

## 返回数据格式

### processMove 返回格式
```json
{
    "valid": true,
    "msg": "",
    "sync_map": [
        [1,2,3,4,5,1,2,3],
        [2,3,4,5,1,2,3,4],
        [3,4,5,1,2,3,4,5],
        [4,5,1,2,3,4,5,1],
        [5,1,2,3,4,5,1,2],
        [1,2,3,4,5,1,2,3],
        [2,3,4,5,1,2,3,4],
        [3,4,5,1,2,3,4,5]
    ],
    "special_layers": {
        "ice_map": [
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false]
        ],
        "bomb_list": [
            {"r":2,"c":3,"timer":10},
            {"r":5,"c":7,"timer":8},
            {"r":0,"c":1,"timer":15}
        ]
    },
    "game_status": {
        "is_over": false,
        "is_win": false,
        "reason": "",
        "moves_left": 15,
        "current_score": 120,
        "target_score": 1000
    },
    "total_score_gained": 30,
    "attack_triggered": false,
    "events": [
        {"type": "swap", "from": [3,4], "to": [3,5]},
        {"type": "eliminate", "coords": [[3,2],[3,3],[3,4],[3,5],[3,6]], "score": 50},
        {"type": "refill", "map": [
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,5,1,2,3,4,5],
            [4,5,1,2,3,4,5,1],
            [5,1,2,3,4,5,1,2],
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,5,1,2,3,4,5]
        ], "ice_map": [
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false]
        ], "bomb_map": [
            {"r":2,"c":3,"timer":9},
            {"r":5,"c":7,"timer":7}
        ]}
    ],
    "coins_earned": 100,
    "new_level_unlocked": true
}
```

### getDualState 返回格式 (PVP/PVE)
```json
{
    "status": "playing",
    "my_score": 150,
    "opp_nickname": "Player123",
    "is_frozen": false,
    "freeze_time_ms": 0,
    "opp_score": 120,
    "opp_events": [
        {"type": "swap", "from": [2,3], "to": [2,4]},
        {"type": "eliminate", "coords": [[2,1],[2,2],[2,3],[2,4],[2,5]], "score": 50}
    ],
    "opp_map": [
        [1,2,3,4,5,1,2,3],
        [2,3,4,5,1,2,3,4],
        [3,4,5,1,2,3,4,5],
        [4,5,1,2,3,4,5,1],
        [5,1,2,3,4,5,1,2],
        [1,2,3,4,5,1,2,3],
        [2,3,4,5,1,2,3,4],
        [3,4,5,1,2,3,4,5]
    ],
    "opp_bomb_list": [
        {"r":1,"c":2,"timer":12},
        {"r":4,"c":6,"timer":5}
    ],
    "opp_is_frozen": false,
    "time_left_sec": 45,
    "is_over": false,
    "is_win": false,
    "coins_earned": 1,
    "new_high_score": true
}
```

## 实现注意事项

1. **线程安全**: 使用互斥锁保护共享数据
2. **内存管理**: 使用shared_ptr管理GameSession生命周期
3. **数据库连接**: 每个操作使用独立的数据库连接
4. **随机数**: 使用稳定的随机数生成器
5. **时间戳**: 使用毫秒级时间戳
6. **事件同步**: PVP模式下需要同步事件到对手

## 测试建议

1. 单元测试地图生成和匹配检测
2. 集成测试完整的游戏流程
3. 并发测试PVP匹配和会话管理
4. 数据库操作的边界情况测试</content>
<parameter name="filePath">c:\Users\敖翔\Desktop\Game_v1_release\Server\SERVICEDEVELOP.md