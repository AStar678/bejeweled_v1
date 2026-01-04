#pragma once
#include "json.hpp"

using json = nlohmann::json;

// 统一响应辅助类
class Response {
public:
    static json success(const json& data = nullptr) {
        json res;
        res["code"] = 200;
        if (data != nullptr) res["data"] = data;
        return res;
    }

    static json error(int code, const std::string& msg) {
        json res;
        res["code"] = code;
        res["msg"] = msg;
        return res;
    }
};//
// Created by 敖翔 on 2025/12/24.
//