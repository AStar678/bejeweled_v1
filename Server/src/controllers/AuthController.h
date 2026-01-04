#pragma once
#include "crow_all.h"
#include "../utils/Response.h"
#include <iostream>
#include <string>
#include <models/User.h>
#include "../dao/UserDao.h"

class AuthController {
private:
    UserDao userDao;

public:
    AuthController() : userDao() {}

    template <typename T>
    void registerRoutes(T& app) {

        //登录接口
        CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string account = body["account"];
            std::string password = body["password"];

            User user;
            if (userDao.login(account, password, user)) {
                json data;
                data["token"] = "mock-token-" + std::to_string(user.uid);
                data["uid"] = user.uid;
                data["nickname"] = user.nickname;
                data["assets"] = user.toAssetsJson();

                return crow::response(Response::success(data).dump());
            }
            return crow::response(401, Response::error(401, "账号或密码错误").dump());
        });

        //注册接口
        CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto body = json::parse(req.body);
            std::string account = body["account"];
            std::string password = body["password"];
            std::string nickname = body.value("nickname", "NewPlayer");

            if (userDao.registerUser(account, password, nickname)) {
                return crow::response(Response::success().dump());
            } else {
                return crow::response(400, Response::error(400, "该账号已存在").dump());
            }
        });

        //同步用户信息接口
        CROW_ROUTE(app, "/api/user/sync").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto body = json::parse(req.body);
            int uid = body.value("uid", 0);

            User user;
            if (userDao.getUserById(uid, user)) {
                json data;
                data["uid"] = uid;
                data["nickname"] = user.nickname;
                data["assets"] = user.toAssetsJson();

                return crow::response(Response::success(data).dump());
            }
            return crow::response(404, Response::error(404, "User not found").dump());
        });
    }
};