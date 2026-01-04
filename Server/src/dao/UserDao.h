#pragma once
#include <QtSql>
#include <QUuid>
#include <string>
#include <nlohmann/json.hpp>
#include "../models/User.h"

using json = nlohmann::json;

class UserDao {
private:
    QSqlDatabase getDBConnection() {
        QString connectionName = QUuid::createUuid().toString();
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("game.db");
        return db;
    }

public:
    UserDao();

    // 初始化数据库表
    void initDBTable();

    // 用户认证相关
    bool login(const std::string& account, const std::string& password, User& user);
    bool registerUser(const std::string& account, const std::string& password, const std::string& nickname);
    bool getUserById(int uid, User& user);

    // 更新用户数据
    bool updateAsset(int uid, const std::string& type, int delta);
    bool updateMaxScore(int uid, int current_score);
    bool updateMaxLevel(int uid, int level_passed);
    std::string getNicknameFromDB(int uid);

    // 排行榜
    json getLeaderboard();
};