#pragma once
#include "crow_all.h"
#include "../services/GameService.h"
#include "../utils/Response.h"

class GameController {
public:
    template <typename T>
    void registerRoutes(T& app) {

        //开始游戏
        CROW_ROUTE(app, "/api/game/start").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string mode = body.value("mode", "level");
            int level = body.value("level", 1);
            int uid = body.value("uid", 0);
            
            //创建游戏对象，后续用uuid来定位游戏
            GameSession* session = GameService::getInstance().createSession(uid, mode, level);

            json data;
            data["game_uuid"] = session->uuid;
            data["map"] = session->map;
            data["ice_map"] = session->ice_map;

            json bombList = json::array();
            for(auto const& [key, val] : session->bomb_map)
                bombList.push_back({ {"r", key/8}, {"c", key%8}, {"timer", val} });
            data["bomb_map"] = bombList;

            data["level_info"] = {
                {"current", level},
                {"target", session->config.target_score},
                {"desc", session->config.desc},
                {"moves", session->moves_left}
            };
            return crow::response(Response::success(data).dump());
        });

        //移动操作
        CROW_ROUTE(app, "/api/game/move").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string uuid = body["game_uuid"];
            int r = body["row"];
            int c = body["col"];
            std::string dir = body["direction"];
            
            //调用游戏对象的移动功能
            json res = GameService::getInstance().processMove(uuid, r, c, dir);
            return crow::response(Response::success(res).dump());
        });

        //PVE 开始
        CROW_ROUTE(app, "/api/pve/start").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            int uid = body.value("uid", 0);
            int diff = body.value("difficulty", 1);

            //调用服务层开始 PVE 对战
            json res = GameService::getInstance().startPVE(uid, diff);
            return crow::response(Response::success(res).dump());
        });

        //PVP 匹配
        CROW_ROUTE(app, "/api/pvp/match").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            int uid = body.value("uid", 0);

            //调用服务层进行 PVP 匹配
            json res = GameService::getInstance().joinPVP(uid);
            return crow::response(Response::success(res).dump());
        });

        //对战状态查询 (PVP/PVE)
        CROW_ROUTE(app, "/api/pvp/status").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string uuid = body["game_uuid"];

            //调用服务层获取对战状态
            json res = GameService::getInstance().getDualState(uuid);
            return crow::response(Response::success(res).dump());
        });

        //商城购买
        CROW_ROUTE(app, "/api/shop/buy").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            int uid = body.value("uid", 0);
            std::string type = body["item_type"];

            //调用服务层处理购买请求
            json res = GameService::getInstance().buyItem(uid, type);
            if (res["code"] == 200) return crow::response(Response::success(res).dump());
            return crow::response(200, Response::error(400, res["msg"].get<std::string>()).dump());
        });

        //使用道具
        CROW_ROUTE(app, "/api/game/use_item").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string uuid = body["game_uuid"];
            std::string type = body["item_type"];
            int r = body.value("row", -1);
            int c = body.value("col", -1);

            //调用服务层处理使用道具请求   
            json res = GameService::getInstance().useItem(uuid, type, r, c);
            if (res["code"] == 200) return crow::response(Response::success(res).dump());
            return crow::response(200, Response::error(400, res["msg"].get<std::string>()).dump());
        });

        //排行榜接口
        CROW_ROUTE(app, "/api/rank").methods(crow::HTTPMethod::GET)
        ([]() {

            //调用服务层获取排行榜数据
            json res = GameService::getInstance().getLeaderboard();
            return crow::response(Response::success(res).dump());
        });


        //退出游戏接口
        CROW_ROUTE(app, "/api/game/quit").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string uuid = body.value("game_uuid", "");
            GameService::getInstance().quitGame(uuid);
            return crow::response(Response::success().dump());
        });

        //取消 PVP 匹配接口
        CROW_ROUTE(app, "/api/pvp/cancel").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            auto body = json::parse(req.body);
            int uid = body.value("uid", 0);

            //调用服务层取消匹配
            bool success = GameService::getInstance().cancelMatch(uid);
            if(success) return crow::response(Response::success().dump());
            else return crow::response(200, Response::error(400, "当前不在匹配队列中").dump());
        });
    }
};