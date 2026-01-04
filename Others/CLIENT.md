# 前端开发文档 - 宝石消除游戏API接口

## 概述

本文档描述了宝石消除游戏的前端API接口规范。前端需要通过HTTP请求与后端服务器通信，服务器运行在 `http://120.46.159.115:8000`。

## 通用规范

### 请求格式
- 所有POST请求使用JSON格式的请求体
- Content-Type: `application/json`

### 响应格式
```json
// 成功响应
{
    "code": 200,
    "data": {
        // 具体数据
    }
}

// 错误响应
{
    "code": 400, // 或其他错误码
    "msg": "错误信息"
}
```

### 认证
- 登录后获取token，后续请求在请求头中携带：`Authorization: Bearer {token}`
- 当前实现使用mock token，格式为 `mock-token-{uid}`

## API接口列表

## 1. 认证相关接口

### 1.1 用户登录
**接口**: `POST /api/auth/login`

**请求参数**:
```json
{
    "account": "用户名",
    "password": "密码"
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "token": "mock-token-123",
        "uid": 123,
        "nickname": "玩家昵称",
        "assets": {
            "coins": 500,  //玩家的金币
            "max_level": 1, //玩家当前的关卡
            "items": {  //玩家的道具数量
                "bomb": 0,
                "reset": 0,
                "freeze": 0
            }
        }
    }
}
```

**错误响应**:
```json
{
    "code": 401,
    "msg": "账号或密码错误"
}
```

### 1.2 用户注册
**接口**: `POST /api/auth/register`

**请求参数**:
```json
{
    "account": "用户名",
    "password": "密码",
    "nickname": "昵称" // 可选，默认"NewPlayer"
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {}
}
```

**错误响应**:
```json
{
    "code": 400,
    "msg": "该账号已存在"
}
```

### 1.3 同步用户信息
用来重新获取一次登录时的信息

**接口**: `POST /api/user/sync`

**请求参数**:
```json
{
    "uid": 123  //看上面
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "uid": 123,
        "nickname": "玩家昵称",
        "assets": {
            "coins": 500,
            "max_score": 1200,
            "max_level": 3,
            "items": {
                "bomb": 2,
                "reset": 1,
                "freeze": 0
            }
        }
    }
}
```

## 2. 游戏相关接口

### 2.1 开始游戏
**接口**: `POST /api/game/start`

**请求参数**:
```json
{
    "mode": "level", // "level" 或 "endless"
    "level": 1,      // 关卡编号 (1-5)
    "uid": 123
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "game_uuid": "game-123-1234567890",
        "map": [
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,5,1,2,3,4,5],
            [4,5,1,2,3,4,5,1],
            [5,1,2,3,4,5,1,2],
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,5,1,2,3,4,5]  //宝石，每个数字代表一个宝石，后端保证不会出现3个连续的宝石
        ],
        "ice_map": [
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false],
            [false,false,false,false,false,false,false,false] //第二关的冰块
        ],
        "bomb_map": [
            {"r":2,"c":3,"timer":15},
            {"r":5,"c":7,"timer":10}
        ],//第三关的炸弹，包括位置和时间
        "level_info": {
            "current": 1,
            "target": 1000,
            "desc": "LV1: 热身运动",
            "moves": -1
        }
    }
}
```

### 2.2 移动操作
**接口**: `POST /api/game/move`

**请求参数**:
```json
{
    "game_uuid": "game-123-1234567890",
    "row": 3,
    "col": 4,
    "direction": "UP" // "UP", "DOWN", "LEFT", "RIGHT" //移动的方块的位置，移动的方向
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "valid": true,
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
                {"r":2,"c":3,"timer":14},
                {"r":5,"c":7,"timer":9}
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
            {"type": "swap", "from": [3,4], "to": [2,4]},
            {"type": "eliminate", "coords": [[2,2],[2,3],[2,4],[2,5],[3,4]], "score": 50},
            {"type": "refill", "map": [...], "ice_map": [...], "bomb_map": [...]}
        ],
        "coins_earned": 0,
        "new_level_unlocked": false
    }
}
```

### 2.3 开始PVE模式
**接口**: `POST /api/pve/start`

**请求参数**:
```json
{
    "uid": 123,
    "difficulty": 1 // 1-简单, 2-普通, 3-困难
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "game_uuid": "pve-p-123-1234567890",
        "ai_uuid": "pve-ai-1234567890",
        "difficulty": 1
    }
}
```

### 2.4 PVP匹配
**接口**: `POST /api/pvp/match`

**请求参数**:
```json
{
    "uid": 123
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "status": "waiting", // 或 "matched"
        "game_uuid": "pvp-123-1234567890",
        "opponent_uid": 456,      // matched时才有
        "opponent_nick": "对手昵称" // matched时才有
    }
}
```

### 2.5 对战状态查询
获取一些对手的信息，用来渲染

**接口**: `POST /api/pvp/status`

**请求参数**:
```json
{
    "game_uuid": "pvp-123-1234567890"
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "status": "playing",
        "my_score": 150,
        "opp_nickname": "对手昵称",
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
}
```

### 2.6 商城购买
**接口**: `POST /api/shop/buy`

**请求参数**:
```json
{
    "uid": 123,
    "item_type": "bomb" // "bomb", "reset", "freeze"
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "code": 200,
        "msg": "购买成功"
    }
}
```

**错误响应**:
```json
{
    "code": 200,
    "data": {
        "code": 400,
        "msg": "金币不足"
    }
}
```

### 2.7 使用道具
**接口**: `POST /api/game/use_item`

**请求参数**:
```json
{
    "game_uuid": "game-123-1234567890",
    "item_type": "bomb", // "bomb", "reset", "freeze"
    "row": 2,            // bomb时需要指定位置
    "col": 3             // bomb时需要指定位置
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {
        "code": 200,
        "msg": "Used bomb",
        "new_map": [
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,0,0,0,3,4,5],
            [4,5,0,0,0,4,5,1],
            [5,1,0,0,0,5,1,2],
            [1,2,3,4,5,1,2,3],
            [2,3,4,5,1,2,3,4],
            [3,4,5,1,2,3,4,5]
        ],
        "events": [
            {"type": "eliminate", "coords": [[2,2],[2,3],[2,4],[3,2],[3,3],[3,4],[4,2],[4,3],[4,4]]},
            {"type": "refill", "map": [...], "ice_map": [...], "bomb_map": [...]}
        ],
        "new_bombs": [
            {"r":5,"c":7,"timer":8}
        ]
    }
}
```

### 2.8 获取排行榜
**接口**: `GET /api/rank`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "data": [
        {"nickname": "玩家1", "score": 5000},
        {"nickname": "玩家2", "score": 4800},
        {"nickname": "玩家3", "score": 4500}
    ]
}
```

### 2.9 退出游戏
**接口**: `POST /api/game/quit`

**请求参数**:
```json
{
    "game_uuid": "game-123-1234567890"
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {}
}
```

### 2.10 取消PVP匹配
**接口**: `POST /api/pvp/cancel`

**请求参数**:
```json
{
    "uid": 123
}
```

**成功响应**:
```json
{
    "code": 200,
    "data": {}
}
```

**错误响应**:
```json
{
    "code": 200,
    "data": {
        "code": 400,
        "msg": "当前不在匹配队列中"
    }
}
```

## 3. 数据格式说明

### 3.1 游戏地图
- 8x8的二维数组
- 数字1-5表示不同颜色的宝石
- 数字0表示空位置
- 数字9表示病毒（某些关卡）

### 3.2 冰块地图
- 8x8的布尔数组
- `true`表示有冰块覆盖
- `false`表示无冰块

### 3.3 炸弹列表
```json
[
    {
        "r": 2,     // 行坐标 (0-7)
        "c": 3,     // 列坐标 (0-7)
        "timer": 10 // 倒计时秒数
    }
]
```

### 3.4 游戏事件
- `swap`: 交换操作 `{"type": "swap", "from": [r,c], "to": [r,c]}`
- `eliminate`: 消除操作 `{"type": "eliminate", "coords": [[r,c],...], "score": 50}`
- `refill`: 填充操作 `{"type": "refill", "map": [...], "ice_map": [...], "bomb_map": [...]}`
- `virus_spread`: 病毒扩散 `{"type": "virus_spread", "cells": [[r,c],...]}`
- `virus_spawn`: 病毒生成 `{"type": "virus_spawn", "cells": [[r,c],...]}`
- `eliminate`: 道具消除 `{"type": "eliminate", "coords": [[r,c],...], "score": 0}`

## 4. 前端实现要点

### 4.1 状态管理
- 维护用户登录状态和token
- 维护当前游戏状态（地图、分数、道具等）
- 处理PVP/PVE的不同状态

### 4.2 游戏模式实现逻辑

#### 4.2.1 单人关卡模式 (Level Mode)
**流程**:
1. 用户选择关卡 → 调用 `/api/game/start` 开始游戏
2. 接收初始地图数据，渲染游戏界面
3. 用户进行宝石交换 → 调用 `/api/game/move`
4. 根据返回的事件队列播放动画效果
5. 检查游戏状态，重复步骤3直到游戏结束
6. 游戏结束时显示结果和奖励

**关键逻辑**:
- 关卡有目标分数和移动次数限制
- 支持冰块、炸弹、病毒等特殊元素
- 道具只能在PVP模式使用

#### 4.2.2 无尽模式 (Endless Mode)
**流程**:
1. 调用 `/api/game/start` 时设置 `mode: "endless"`
2. 游戏无移动次数限制，无目标分数
3. 每回合获得的金币 = 分数 ÷ 100
4. 游戏可随时退出，最高分会记录到数据库

#### 4.2.3 PVE模式实现
**流程**:
1. 用户选择难度 → 调用 `/api/pve/start`
2. 获得玩家和AI的双人游戏UUID
3. 开始游戏循环：
   - 玩家进行移动 → 调用 `/api/game/move`
   - 定期调用 `/api/pvp/status` 获取AI状态和事件
   - 处理AI的事件队列，播放对手动画
4. AI会自动进行移动，无需手动触发

**同步机制**:
- **轮询方式**: 频繁调用 `/api/pvp/status`
- **事件驱动**: 对手的每次移动都会产生事件，通过status接口获取
- **状态同步**: 实时同步对手分数、地图状态、冻结状态等

#### 4.2.4 PVP模式实现
**流程**:
1. 用户点击匹配 → 调用 `/api/pvp/match`
2. 如果返回 `status: "waiting"`，显示等待界面，继续轮询
3. 匹配成功后，获得对手信息，开始游戏
4. 游戏过程中：
   - 双方都可以随时移动
   - 定期同步双方状态
   - 处理攻击效果（高分移动会冻结对手）
5. 60秒倒计时结束或一方认输时游戏结束

**同步机制**:
- **实时轮询**: 频繁调用 `/api/pvp/status`
- **双向事件**: 获取对手的事件队列，同时自己的操作也会影响对手
- **状态监控**: 监控冻结状态、分数变化、游戏结束状态
- **断线处理**: 如果 `status: "opponent_left"`，显示对手离开


### 4.3 道具系统实现

#### 4.3.1 炸弹道具
- 点击道具按钮，选择目标位置
- 调用 `/api/game/use_item` 带位置参数
- 服务器返回3x3范围的消除事件
- 前端播放爆炸特效和消除动画

#### 4.3.2 重置道具
- 直接调用 `/api/game/use_item`，无需位置
- 服务器重新生成地图
- 前端直接更新整个地图

#### 4.3.3 冻结道具
- 对PVP对手使用，调用 `/api/game/use_item`
- 对手3秒内无法移动
- 前端显示冻结特效和倒计时


