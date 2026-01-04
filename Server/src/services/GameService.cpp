#include "GameService.h"

// 单例实现
GameService& GameService::getInstance() {
    static GameService instance;
    return instance;
}

nlohmann::json GameService::getLeaderboard() {
    return userDao.getLeaderboard();
}


// --- 游戏核心逻辑 ---

void GameService::generateMap(GameSession& session) {
    session.moves_left = -1; // 默认无限步

    // 根据关卡配置难度
    if (session.mode == "level") {
        if (session.level == 2) session.config.ice_count = 10;
        if (session.level == 3) session.config.bomb_count = 5;
        if (session.level == 4) session.config.virus_count = 3;
        if (session.level == 5) { 
            // 第5关是挑战关，没特殊元素但步数少
            session.config.ice_count = 0; 
            session.config.bomb_count = 0; 
            session.config.virus_count = 0; 
            session.moves_left = 20; 
        }
    }

    // 1. 生成基础宝石矩阵，保证初始没有 3 连消除
    static std::random_device rd; 
    static std::mt19937 gen(rd()); 
    static std::uniform_int_distribution<> dis(1, 5);

    for(int r = 0; r < 8; ++r) { 
        for(int c = 0; c < 8; ++c) { 
            int gem;
            while(true) { 
                gem = dis(gen); 
                // 检查左边两个是否一样
                bool matchH = (c >= 2 && session.map[r][c-1] == gem && session.map[r][c-2] == gem);
                // 检查上边两个是否一样
                bool matchV = (r >= 2 && session.map[r-1][c] == gem && session.map[r-2][c] == gem);
                
                if(!matchH && !matchV) break; // 安全，可以用这个颜色
            } 
            session.map[r][c] = gem; 
        } 
    }

    // 2. 冰块
    const auto& cfg = session.config;
    for(int i = 0; i < cfg.ice_count; ++i) { 
        int r, c, retry = 0; 
        do { 
            r = randomInt(0, 7); 
            c = randomInt(0, 7); 
            retry++; 
        } while(session.ice_map[r][c] && retry < 20); // 别重复
        session.ice_map[r][c] = true; 
    }

    // 3. 炸弹
    for(int i = 0; i < cfg.bomb_count; ++i) { 
        int r, c, retry = 0; 
        do { 
            r = randomInt(0, 7); 
            c = randomInt(0, 7); 
            retry++; 
        } while(session.bomb_map.count(GameSession::posKey(r,c)) && retry < 20); 
        session.bomb_map[GameSession::posKey(r,c)] = cfg.bomb_initial_time; 
    }

    // 4. 病毒 (病毒会覆盖掉原来的宝石和特殊物)
    for(int i = 0; i < cfg.virus_count; ++i) { 
        int r, c; 
        do { 
            r = randomInt(0, 7); 
            c = randomInt(0, 7); 
        } while(session.map[r][c] == 9); // 别覆盖已经是病毒的
        
        session.map[r][c] = 9; 
        session.ice_map[r][c] = false; // 病毒上没冰
        session.bomb_map.erase(GameSession::posKey(r,c)); // 病毒位置没炸弹
    }
}

// Match-3 检查算法
std::set<Point> GameService::findMatches(const std::vector<std::vector<int>>& map) { 
    std::set<Point> matched; 
    
    // 查横向
    for(int r = 0; r < 8; r++) {
        for(int c = 0; c < 6; c++) { 
            int k = map[r][c]; 
            if(k <= 0 || k == 9) continue; // 空格或病毒不消除
            if(map[r][c+1] == k && map[r][c+2] == k) {
                matched.insert({r, c});
                matched.insert({r, c+1});
                matched.insert({r, c+2});
            }
        }
    } 
    
    // 查纵向
    for(int c = 0; c < 8; c++) {
        for(int r = 0; r < 6; r++) { 
            int k = map[r][c]; 
            if(k <= 0 || k == 9) continue; 
            if(map[r+1][c] == k && map[r+2][c] == k) {
                matched.insert({r, c});
                matched.insert({r+1, c});
                matched.insert({r+2, c});
            }
        }
    } 
    return matched; 
}

// 处理消除时的附带效果（破冰、炸弹倒计时、炸病毒）
void GameService::handleSpecialEliminations(GameSession& s, std::set<Point>& m) { 
    std::set<Point> extra; // 额外要消除的点（比如炸到了旁边的病毒）
    
    for(const auto& p : m) { 
        // 破冰
        if(s.ice_map[p.r][p.c]) {
            s.ice_map[p.r][p.c] = false; 
        }
        
        // 消除该位置的炸弹（如果有）
        int k = GameSession::posKey(p.r, p.c); 
        if(s.bomb_map.count(k)) {
            s.bomb_map.erase(k); 
        }

        // 检查上下左右有没有病毒，有的话一起带走
        int dr[] = {-1, 1, 0, 0};
        int dc[] = {0, 0, -1, 1}; 
        for(int i = 0; i < 4; ++i) { 
            int nr = p.r + dr[i];
            int nc = p.c + dc[i]; 
            if(nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && s.map[nr][nc] == 9) {
                extra.insert({nr, nc}); 
            } 
        } 
    } 
    // 合并
    m.insert(extra.begin(), extra.end()); 
}

// 消除 -> 下落 -> 补充逻辑
void GameService::applyElimination(GameSession& s, const std::set<Point>& m) {
    // 1. 把消除的点置为 0 (空)
    for(const auto& p : m) { 
        s.map[p.r][p.c] = 0; 
        s.ice_map[p.r][p.c] = false; 
        s.bomb_map.erase(GameSession::posKey(p.r, p.c)); 
    }
    
    // 2. 准备补充池 (refill pool)
    // 根据当前关卡目标，如果冰块/炸弹被消没了，可能需要重新生成一些
    struct NewItem { bool ice; int bomb; }; 
    std::vector<NewItem> refill_pool; 
    
    int total_empty = 0;
    for(int c=0; c<8; c++) 
        for(int r=0; r<8; r++) 
            if(s.map[r][c] == 0) total_empty++;

    int ice_needed = 0; 
    int bomb_needed = 0;
    
    if (s.mode == "level") {
        if (s.level == 2) { 
            int cur = countIce(s); 
            if (cur < s.config.ice_count) ice_needed = s.config.ice_count - cur; 
        }
        if (s.level == 3) { 
            int cur = countBombs(s); 
            if (cur < s.config.bomb_count) bomb_needed = s.config.bomb_count - cur; 
        }
    }
    
    for(int i=0; i<ice_needed; i++) refill_pool.push_back({true, -1});
    for(int i=0; i<bomb_needed; i++) refill_pool.push_back({false, s.config.bomb_initial_time});
    
    // 剩下的都填普通宝石
    while(refill_pool.size() < total_empty) refill_pool.push_back({false, -1});
    
    // 打乱补充池
    static std::random_device rd; 
    static std::mt19937 g(rd()); 
    std::shuffle(refill_pool.begin(), refill_pool.end(), g);
    
    int pool_idx = 0;

    // 3. 处理每一列的下落
    for(int c = 0; c < 8; c++) {
        struct Cell { int v; int b; bool i; }; 
        std::vector<Cell> col_data;
        
        // 收集这一列还存在的方块
        for(int r = 0; r < 8; r++) { 
            int k = GameSession::posKey(r,c); 
            if(s.map[r][c] != 0) { 
                int t = -1; 
                if(s.bomb_map.count(k)) t = s.bomb_map[k]; 
                col_data.push_back({s.map[r][c], t, (bool)s.ice_map[r][c]}); 
            } 
            // 清理旧位置
            s.bomb_map.erase(k); 
        }
        
        // 计算缺了几个
        int missing = 8 - col_data.size();
        
        // 在顶部插入新的
        for(int k = 0; k < missing; k++) { 
            NewItem item = {false, -1}; 
            if (pool_idx < refill_pool.size()) item = refill_pool[pool_idx++]; 
            
            // 插入到最前面（模拟从上面掉下来）
            col_data.insert(col_data.begin(), {randomGem(), item.bomb, item.ice}); 
        }
        
        // 写回地图
        for(int r = 0; r < 8; r++) { 
            s.map[r][c] = col_data[r].v; 
            s.ice_map[r][c] = col_data[r].i; 
            if(col_data[r].b != -1) {
                s.bomb_map[GameSession::posKey(r,c)] = col_data[r].b; 
            }
        }
    }
}

nlohmann::json GameService::forceSpawnViruses(GameSession& s, int count) {
    nlohmann::json cells = nlohmann::json::array();
    int spawned = 0; 
    int attempts = 0;
    
    while(spawned < count && attempts < 100) {
        attempts++;
        int r = randomInt(0, 7); 
        int c = randomInt(0, 7);
        int k = GameSession::posKey(r, c);
        
        // 找个干净的地方放病毒
        bool isClean = (s.map[r][c] != 9 && s.map[r][c] != 0 && !s.ice_map[r][c] && s.bomb_map.find(k) == s.bomb_map.end());
        
        if (isClean) {
            s.map[r][c] = 9; 
            s.ice_map[r][c] = false; 
            s.bomb_map.erase(k);
            cells.push_back({{"r", r}, {"c", c}}); 
            spawned++;
        }
    }
    return cells;
}

nlohmann::json GameService::spreadVirus(GameSession& s) { 
    nlohmann::json new_virus = nlohmann::json::array(); 
    std::vector<Point> sources; 
    
    // 找到所有现存病毒
    for(int r=0; r<8; ++r) 
        for(int c=0; c<8; ++c) 
            if(s.map[r][c] == 9) sources.push_back({r, c}); 
            
    // 每个病毒有概率传染周围
    for(auto p : sources) { 
        if(randomInt(1, 100) <= 50) { // 50% 概率传染
            int dr[]={-1,1,0,0}, dc[]={0,0,-1,1};
            int d = randomInt(0, 3);
            int nr = p.r + dr[d];
            int nc = p.c + dc[d]; 
            
            if(nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && s.map[nr][nc] != 9) { 
                s.map[nr][nc] = 9; 
                s.ice_map[nr][nc] = false; 
                s.bomb_map.erase(GameSession::posKey(nr, nc)); 
                new_virus.push_back({{"r", nr}, {"c", nc}}); 
            } 
        } 
    } 
    return new_virus; 
}

// --- 会话与匹配管理 ---

GameSession* GameService::createSession(int uid, const std::string& mode, int level) {
    std::lock_guard<std::mutex> l(session_mutex);
    std::string nick = userDao.getNicknameFromDB(uid);
    std::string id = "game-" + std::to_string(uid) + "-" + std::to_string(rand());
    
    auto s = std::make_shared<GameSession>(id, uid, nick, mode, level);
    generateMap(*s);
    sessions[id] = s;
    return s.get(); // 返回原始指针供外部简单使用，但生命周期由 sessions 持有
}

std::shared_ptr<GameSession> GameService::getSession(const std::string& uuid) {
    std::lock_guard<std::mutex> l(session_mutex);
    if(sessions.find(uuid) != sessions.end()) return sessions[uuid];
    return nullptr;
}

nlohmann::json GameService::startPVE(int uid, int diff) {
    std::lock_guard<std::mutex> l(session_mutex);
    std::string nick = userDao.getNicknameFromDB(uid);
    std::string pid = "pve-p-" + std::to_string(uid) + "-" + std::to_string(rand());
    
    // 玩家 Session
    auto ps = std::make_shared<GameSession>(pid, uid, nick, "pve", 1);
    ps->is_pvp = true; 
    generateMap(*ps);

    // AI Session
    std::string aid = "pve-ai-" + std::to_string(rand());
    auto as = std::make_shared<GameSession>(aid, 0, "Bot", "pve", 1);
    as->is_pvp = true; 
    as->is_ai = true; 
    as->ai_difficulty = diff; 
    generateMap(*as);

    // 互相关联
    ps->opponent_uuid = aid; ps->opponent_nickname = "Bot";
    as->opponent_uuid = pid; as->opponent_nickname = nick;

    long long t = nowMs(); 
    ps->start_time = t; 
    as->start_time = t;
    
    sessions[pid] = ps; 
    sessions[aid] = as;
    
    return {{"game_uuid", pid}, {"ai_uuid", aid}, {"difficulty", diff}};
}

nlohmann::json GameService::joinPVP(int uid) {
    std::lock_guard<std::mutex> l(session_mutex);

    // 检查自己是不是已经在排队了（防止狂点匹配）
    if(!waiting_pvp_uuid.empty()) {
        if(sessions.count(waiting_pvp_uuid) && sessions[waiting_pvp_uuid]->uid == uid) {
             return {{"status", "waiting"}, {"game_uuid", waiting_pvp_uuid}};
        }
    }
    
    std::string nick = userDao.getNicknameFromDB(uid);
    std::string mid = "pvp-" + std::to_string(uid) + "-" + std::to_string(rand());
    auto ms = std::make_shared<GameSession>(mid, uid, nick, "pvp", 1);
    generateMap(*ms);

    if(waiting_pvp_uuid.empty() || sessions.find(waiting_pvp_uuid) == sessions.end()) {
        // 没人排队，我先进去等
        waiting_pvp_uuid = mid;
        sessions[mid] = ms;
        return {{"status", "waiting"}, {"game_uuid", mid}};
    } else {
        // 匹配成功！
        std::string oid = waiting_pvp_uuid;
        auto os = sessions[oid]; // 对手 session

        // 互相关联
        ms->opponent_uuid = oid; ms->opponent_nickname = os->nickname;
        os->opponent_uuid = mid; os->opponent_nickname = ms->nickname;

        long long t = nowMs();
        ms->start_time = t; 
        os->start_time = t;
        
        sessions[mid] = ms;
        waiting_pvp_uuid = ""; // 清空排队池
        
        return {
            {"status", "matched"},
            {"game_uuid", mid},
            {"opponent_uid", os->uid},
            {"opponent_nick", os->nickname}
        };
    }
}

bool GameService::cancelMatch(int uid) {
    std::lock_guard<std::mutex> l(session_mutex);
    // 只有当前排队的人是自己时才能取消
    if (!waiting_pvp_uuid.empty() && sessions.count(waiting_pvp_uuid) && sessions[waiting_pvp_uuid]->uid == uid) {
        sessions.erase(waiting_pvp_uuid);
        waiting_pvp_uuid = "";
        return true;
    }
    return false;
}

void GameService::quitGame(const std::string& uuid) {
    std::lock_guard<std::mutex> l(session_mutex);
    if (!sessions.count(uuid)) return;
    auto s = sessions[uuid];

    // 如果正在排队，清空
    if (waiting_pvp_uuid == uuid) waiting_pvp_uuid = "";

    // 需要通知对手我跑了
    if (s->is_pvp && !s->opponent_uuid.empty()) {
        if (sessions.count(s->opponent_uuid)) {
            auto opp = sessions[s->opponent_uuid];
            opp->opponent_quit = true;
        }
    }

    sessions.erase(uuid);
    std::cout << "[Info] Session quit: " << uuid << std::endl;
}

// --- 道具逻辑 ---

nlohmann::json GameService::buyItem(int uid, const std::string& itemType) {
    int cost = 0; 
    std::string dbField = "";
    
    if (itemType == "bomb") { cost = GameConfig::ITEM_COST_BOMB; dbField = "item_bomb"; }
    else if (itemType == "reset") { cost = GameConfig::ITEM_COST_RESET; dbField = "item_reset"; }
    else if (itemType == "freeze") { cost = GameConfig::ITEM_COST_FREEZE; dbField = "item_freeze"; }
    else return {{"code", 400}, {"msg", "未知道具"}};
    
    // 扣钱
    if (!userDao.updateAsset(uid, "coins", -cost)) return {{"code", 400}, {"msg", "金币不足"}};
    
    // 加道具 (如果数据库炸了要回滚钱)
    if (!userDao.updateAsset(uid, dbField, 1)) { 
        userDao.updateAsset(uid, "coins", cost); 
        return {{"code", 500}, {"msg", "数据库错误: 无法发货"}}; 
    }
    return {{"code", 200}, {"msg", "购买成功"}};
}

nlohmann::json GameService::useItem(const std::string& uuid, const std::string& itemType, int r, int c) {
    auto session = getSession(uuid);
    if (!session) return {{"code", 404}, {"msg", "Session not found"}};
    if (session->is_ai) return {{"code", 400}, {"msg", "PVE模式不可使用道具"}};
    if (!session->is_pvp) return {{"code", 400}, {"msg", "道具只能在 PVP/PVE 中使用"}};
    if (session->is_over) return {{"code", 400}, {"msg", "游戏已结束"}};

    std::string dbField = "";
    if (itemType == "bomb") dbField = "item_bomb";
    else if (itemType == "reset") dbField = "item_reset";
    else if (itemType == "freeze") dbField = "item_freeze";
    else return {{"code", 400}, {"msg", "未知道具"}};

    // 扣库存
    if (!userDao.updateAsset(session->uid, dbField, -1)) return {{"code", 400}, {"msg", "道具数量不足"}};

    nlohmann::json events = nlohmann::json::array(); // 记录事件发给前端播放动画

    if (itemType == "reset") {
        // 重置整个地图
        generateMap(*session);
        // 重置没有动画事件，直接前端刷整个 map 即可，或者可以给个特效事件
    }
    else if (itemType == "freeze") {
        // 冻结对手
        if (!session->opponent_uuid.empty()) {
            auto opp = getSession(session->opponent_uuid);
            // 检查对手是否在无敌时间内
            if (opp && nowMs() > opp->immunity_until) {
                opp->frozen_until = nowMs() + GameConfig::FREEZE_DURATION_MS;
                opp->immunity_until = opp->frozen_until + 5000; // 冻结后给点保护期
            }
        }
    }
    else if (itemType == "bomb") {
        if (r < 0 || r >= 8 || c < 0 || c >= 8) return {{"code", 400}, {"msg", "无效的炸弹位置"}};

        // 1. 记录炸弹消除区域 (3x3)
        nlohmann::json bombCoords = nlohmann::json::array();
        for (int nr = r - 1; nr <= r + 1; ++nr) {
            for (int nc = c - 1; nc <= c + 1; ++nc) {
                if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                    session->map[nr][nc] = 0;
                    session->ice_map[nr][nc] = false;
                    session->bomb_map.erase(GameSession::posKey(nr, nc));
                    bombCoords.push_back({nr, nc});
                }
            }
        }
        events.push_back({{"type", "eliminate"}, {"coords", bombCoords}});

        // 2. 填充并记录
        std::set<Point> emptyMatches; // 空集，因为炸弹后只是填充，暂不处理消除
        applyElimination(*session, emptyMatches);

        nlohmann::json cbs = nlohmann::json::array();
        for(auto const&[k,v]:session->bomb_map) cbs.push_back({{"r",k/8},{"c",k%8},{"timer",v}});
        events.push_back({{"type", "refill"}, {"map", session->map}, {"ice_map", session->ice_map}, {"bomb_map", cbs}});

        // 3. 炸弹落下后可能会引发连锁消除，循环处理
        while(true) {
            auto ms = findMatches(session->map);
            if(ms.empty()) break;

            handleSpecialEliminations(*session, ms);
            session->current_score += (ms.size() * 10);

            // 记录连锁消除
            nlohmann::json ecs = nlohmann::json::array();
            for(auto p : ms) ecs.push_back({p.r, p.c});
            events.push_back({{"type", "eliminate"}, {"coords", ecs}, {"score", (int)ms.size() * 10}});

            applyElimination(*session, ms);

            // 记录连锁填充
            cbs = nlohmann::json::array();
            for(auto const&[k,v]:session->bomb_map) cbs.push_back({{"r",k/8},{"c",k%8},{"timer",v}});
            events.push_back({{"type", "refill"}, {"map", session->map}, {"ice_map", session->ice_map}, {"bomb_map", cbs}});
        }
    }

    // 把动画同步给对手（如果存在）
    if (!session->opponent_uuid.empty()) {
        auto opp = getSession(session->opponent_uuid);
        if (opp) {
            std::lock_guard<std::mutex> lock(*opp->event_mutex);
            for (const auto& ev : events) opp->event_queue.push_back(ev);
        }
    }

    nlohmann::json res;
    res["code"] = 200;
    res["msg"] = "Used " + itemType;
    res["new_map"] = session->map;
    res["events"] = events;

    nlohmann::json bList = nlohmann::json::array();
    for(auto const&[k,v]:session->bomb_map) bList.push_back({{"r",k/8},{"c",k%8},{"timer",v}});
    res["new_bombs"] = bList;
    return res;
}

// --- AI 逻辑 ---

std::vector<Move> GameService::getAllMoves(const std::vector<std::vector<int>>& map) { 
    std::vector<Move> moves; 
    // 暴力枚举所有可能的交换
    for(int r=0; r<8; ++r) {
        for(int c=0; c<8; ++c) { 
            // 试着向右换
            if(c < 7) { 
                auto cp = map; 
                std::swap(cp[r][c], cp[r][c+1]); 
                auto m = findMatches(cp); 
                if(!m.empty()) moves.push_back({r, c, "RIGHT", (int)m.size()}); 
            } 
            // 试着向下换
            if(r < 7) { 
                auto cp = map; 
                std::swap(cp[r][c], cp[r+1][c]); 
                auto m = findMatches(cp); 
                if(!m.empty()) moves.push_back({r, c, "DOWN", (int)m.size()}); 
            } 
        }
    } 
    return moves; 
}

void GameService::updateAI(GameSession& ai) {
    long long now = nowMs();
    if(now < ai.frozen_until) return; // AI 被冻结了

    // AI 在游戏开始 3 秒内不行动 (发呆)
    if (now - ai.start_time < 3000) return;

    // AI 思考间隔
    long long cd = GameConfig::AI_DELAY_EASY;
    if(ai.ai_difficulty == 2) cd = GameConfig::AI_DELAY_NORMAL;
    if(ai.ai_difficulty == 3) cd = GameConfig::AI_DELAY_HARD;

    if(now - ai.last_ai_move_time < cd) return;

    auto ms = getAllMoves(ai.map);
    if(ms.empty()) { 
        // 死局了，重置地图
        generateMap(ai); 
        return; 
    }
    
    Move bm = ms[0];
    if(ai.ai_difficulty == 1) {
        // 简单 AI：瞎走一个能消的
        bm = ms[randomInt(0, ms.size() - 1)];
    } else {
        // 困难 AI：选消除最多的
        std::sort(ms.begin(), ms.end(), [](const Move& a, const Move& b){ return a.score > b.score; });
        // 普通 AI 偶尔会失误（不选最优，选第二优）
        bm = (ai.ai_difficulty == 2 && ms.size() > 1 && randomInt(0, 100) < 30) ? ms[1] : ms[0];
    }
    
    processMove(ai.uuid, bm.r, bm.c, bm.dir);
    ai.last_ai_move_time = now;
}


// --- 核心状态轮询 ---

nlohmann::json GameService::getDualState(const std::string& uuid) {
    auto s = getSession(uuid); 
    if(!s) return {{"status", "error"}, {"msg", "Session lost"}};
    
    if(s->opponent_quit) { 
        s->is_over = true; 
        return {{"status", "opponent_left"}}; 
    }

    nlohmann::json res;
    // 等人中...
    if(s->mode == "pvp" && !s->is_ai && s->opponent_uuid.empty()) { 
        res["status"] = "waiting"; 
        return res; 
    }
    
    auto o = getSession(s->opponent_uuid);
    if(!o) { 
        res["status"] = "opponent_left"; 
        s->is_over = true; 
        return res; 
    }

    // 顺便驱动一下 AI
    if(o->is_ai && !o->is_over) updateAI(*o);

    res["status"] = "playing";
    res["my_score"] = s->current_score;
    res["opp_nickname"] = s->opponent_nickname;
    
    long long now = nowMs();
    res["is_frozen"] = (now < s->frozen_until); 
    res["freeze_time_ms"] = (now < s->frozen_until ? s->frozen_until - now : 0);
    res["opp_score"] = o->current_score;

    // 获取并清空这一帧收到的事件（比如对手用了道具产生的动画）
    {
        std::lock_guard<std::mutex> lock(*s->event_mutex);
        res["opp_events"] = s->event_queue;
        s->event_queue.clear();
    }

    // 对手盘面信息（用于显示小窗口）
    res["opp_map"] = o->map;
    nlohmann::json obs = nlohmann::json::array(); 
    for(auto const&[k,v] : o->bomb_map) 
        obs.push_back({{"r", k/8}, {"c", k%8}, {"timer", v}});
    res["opp_bomb_list"] = obs; 
    res["opp_is_frozen"] = (now < o->frozen_until);

    // 时间判定
    long long elapsed = now - s->start_time; 
    long long left = 60000 - elapsed; // 60秒一局
    
    if(left <= 0 && !s->is_over) {
        s->is_over = true; 
        o->is_over = true;
        
        // 结算输赢
        if(s->current_score > o->current_score) s->is_win = true; 
        else if(o->current_score > s->current_score) o->is_win = true;
        
        // 结算奖励
        int reward = s->current_score / GameConfig::COIN_DIVISOR_ENDLESS;
        if(reward > 0) userDao.updateAsset(s->uid, "coins", reward);
        
        if (s->is_pvp || s->mode == "endless") { 
            if (userDao.updateMaxScore(s->uid, s->current_score)) res["new_high_score"] = true; 
        }
    }
    
    res["time_left_sec"] = (left > 0 ? left/1000 : 0); 
    res["is_over"] = s->is_over; 
    res["is_win"] = s->is_win;
    if(s->is_over) res["coins_earned"] = (s->current_score / GameConfig::COIN_DIVISOR_ENDLESS);
    
    return res;
}

// --- 处理移动 ---

nlohmann::json GameService::processMove(const std::string& uuid, int row, int col, const std::string& direction) {
    auto session = getSession(uuid); 

    // 构建返回状态的 Lambda，省得每次 return 都写一遍
    auto buildState = [&](bool valid, const std::string& msg = "") {
        nlohmann::json res; 
        res["valid"] = valid; 
        if(!msg.empty()) res["msg"] = msg;
        if (!session) return res;
        
        res["sync_map"] = session->map; 
        nlohmann::json bl = nlohmann::json::array();
        for(const auto&[k,v] : session->bomb_map) 
            bl.push_back({{"r", k/8}, {"c", k%8}, {"timer", v}});
        
        res["special_layers"] = {{"ice_map", session->ice_map}, {"bomb_list", bl}};
        res["game_status"] = {
            {"is_over", session->is_over},
            {"is_win", session->is_win},
            {"reason", session->end_reason},
            {"moves_left", session->moves_left},
            {"current_score", session->current_score},
            {"target_score", session->config.target_score}
        };
        return res;
    };

    if(!session || session->is_over) return buildState(false, "Game Over");
    if(session->is_pvp && nowMs() < session->frozen_until) return buildState(false, "FROZEN");
    if (direction == "INIT") return buildState(true, "Init");

    // 计算目标位置
    int tr = row, tc = col;
    if(direction == "UP") tr--; 
    else if(direction == "DOWN") tr++; 
    else if(direction == "LEFT") tc--; 
    else if(direction == "RIGHT") tc++; 
    else return buildState(false);

    // 越界检查
    if(tr < 0 || tr >= 8 || tc < 0 || tc >= 8) return buildState(false, "Out");
    
    // 阻挡检查：冰块或者病毒不能换
    if(session->ice_map[row][col] || session->ice_map[tr][tc] || 
       session->map[row][col] == 9 || session->map[tr][tc] == 9) 
       return buildState(false, "Blocked");

    // 执行交换
    std::swap(session->map[row][col], session->map[tr][tc]);
    
    // 交换炸弹 Map 中的数据
    int k1 = GameSession::posKey(row, col);
    int k2 = GameSession::posKey(tr, tc);
    int t1 = session->bomb_map.count(k1) ? session->bomb_map[k1] : -1; 
    int t2 = session->bomb_map.count(k2) ? session->bomb_map[k2] : -1;
    session->bomb_map.erase(k1); 
    session->bomb_map.erase(k2); 
    if(t1 != -1) session->bomb_map[k2] = t1; 
    if(t2 != -1) session->bomb_map[k1] = t2;

    nlohmann::json events = nlohmann::json::array();
    events.push_back({{"type", "swap"}, {"from", {row, col}}, {"to", {tr, tc}}});

    bool has_elim = false; 
    int combo = 0; 
    int round_score = 0;

    // 核心消除循环（连消）
    while(true) {
        auto ms = findMatches(session->map);
        if(ms.empty()) {
            if(!has_elim) {
                // 如果第一次交换就没消除，那得换回去（非法操作）
                std::swap(session->map[row][col], session->map[tr][tc]);
                // 炸弹也要换回去
                session->bomb_map.erase(k1); session->bomb_map.erase(k2); 
                if(t1 != -1) session->bomb_map[k1] = t1; 
                if(t2 != -1) session->bomb_map[k2] = t2;
                return buildState(false, "No match");
            }
            break; // 没得消了，退出循环
        }
        
        has_elim = true; 
        combo++;
        
        handleSpecialEliminations(*session, ms);
        
        int s = ms.size() * 10 * combo; // 简单的连击加分公式
        session->current_score += s; 
        round_score += s;
        
        nlohmann::json ecs = nlohmann::json::array(); 
        for(auto p : ms) ecs.push_back({p.r, p.c});
        
        events.push_back({{"type", "eliminate"}, {"coords", ecs}, {"score", s}});
        
        // 消除并下落补充
        applyElimination(*session, ms);
        
        // 记录新的炸弹位置（因为下落了）
        nlohmann::json cbs = nlohmann::json::array(); 
        for(auto const&[k,v] : session->bomb_map) 
            cbs.push_back({{"r", k/8}, {"c", k%8}, {"timer", v}});
            
        events.push_back({{"type", "refill"}, {"map", session->map}, {"ice_map", session->ice_map}, {"bomb_map", cbs}});
    }

    // PVP 攻击逻辑：单回合分数过高则冻结对手
    if(session->is_pvp && round_score > 80 && !session->opponent_uuid.empty()) {
        auto opp = getSession(session->opponent_uuid);
        if(opp && nowMs() > opp->immunity_until) {
            opp->frozen_until = nowMs() + 3000;
            opp->immunity_until = opp->frozen_until + 5000;
        }
    }

    // 步数扣除
    if(session->moves_left > 0) {
        session->moves_left--;
        if(session->moves_left == 0 && session->current_score < session->config.target_score) { 
            session->is_over = true; 
            session->end_reason = "Out of Moves"; 
        }
    }
    
    // 炸弹倒计时
    for(auto it = session->bomb_map.begin(); it != session->bomb_map.end(); ) { 
        it->second--; 
        if(it->second <= 0) {
            session->is_over = true; 
            session->end_reason = "Bomb Exploded"; 
            ++it;
        } else {
            ++it;
        }
    }

    // 病毒扩散逻辑
    if(session->config.virus_count > 0 && !session->is_over) {
        auto ves = spreadVirus(*session);
        if(!ves.empty()) events.push_back({{"type", "virus_spread"}, {"cells", ves}});
        
        // 第4关特殊逻辑：病毒不够了强制生成，增加难度
        if (session->mode == "level" && session->level == 4) {
            int current_v = countViruses(*session);
            if (current_v < 2) {
                auto new_vs = forceSpawnViruses(*session, 2);
                if (!new_vs.empty()) events.push_back({{"type", "virus_spawn"}, {"cells", new_vs}});
            }
        }
    }

    // 胜负与奖励检查
    bool new_unlock = false; 
    int coins_gained = 0;
    
    if(!session->is_over && session->current_score >= session->config.target_score && !session->is_pvp && session->mode != "endless") {
        session->is_over = true; 
        session->is_win = true; 
        session->end_reason = "Target Reached";
        
        int base_reward = GameConfig::COIN_REWARD_LEVEL_PASS; 
        userDao.updateAsset(session->uid, "coins", base_reward); 
        coins_gained += base_reward;
        
        if (session->mode == "level") { 
            if (userDao.updateMaxLevel(session->uid, session->level)) { 
                new_unlock = true; 
                coins_gained += 100; 
            } 
        }
    }

    if (session->is_pvp || session->mode == "endless") {
        userDao.updateMaxScore(session->uid, session->current_score);
    }

    // 同步事件给对手
    if (!session->opponent_uuid.empty()) {
        auto opp = getSession(session->opponent_uuid);
        if (opp) {
            std::lock_guard<std::mutex> lock(*opp->event_mutex);
            for (const auto& ev : events) opp->event_queue.push_back(ev);
        }
    }

    nlohmann::json res = buildState(true);
    res["total_score_gained"] = round_score;
    res["attack_triggered"] = (round_score > 80);
    res["events"] = events;

    if(session->is_over && session->is_win && !session->is_pvp) { 
        res["coins_earned"] = coins_gained; 
        res["new_level_unlocked"] = new_unlock; 
    }
    return res;
}