#include "UserDao.h"

UserDao::UserDao() {
    initDBTable();
}

void UserDao::initDBTable() {
    QSqlDatabase db = getDBConnection();
    if (db.open()) {
        QSqlQuery query(db);
        bool success = query.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "uid INTEGER PRIMARY KEY AUTOINCREMENT, "
            "account TEXT UNIQUE NOT NULL, "
            "password TEXT NOT NULL, "
            "nickname TEXT DEFAULT 'Player', "
            "coins INTEGER DEFAULT 500, "
            "item_bomb INTEGER DEFAULT 0, "
            "item_reset INTEGER DEFAULT 0, "
            "item_freeze INTEGER DEFAULT 0, "
            "max_score INTEGER DEFAULT 0, "
            "max_level INTEGER DEFAULT 1, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        );
        if (success) {
            query.exec("ALTER TABLE users ADD COLUMN max_level INTEGER DEFAULT 1");
            query.exec("ALTER TABLE users ADD COLUMN max_score INTEGER DEFAULT 0");
            query.exec("ALTER TABLE users ADD COLUMN item_bomb INTEGER DEFAULT 0");
            query.exec("ALTER TABLE users ADD COLUMN item_reset INTEGER DEFAULT 0");
            query.exec("ALTER TABLE users ADD COLUMN item_freeze INTEGER DEFAULT 0");
        }
    }
    db.close();
}

bool UserDao::login(const std::string& account, const std::string& password, User& user) {
    QSqlDatabase db = getDBConnection();
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE account = :acc AND password = :pwd");
    query.bindValue(":acc", QString::fromStdString(account));
    query.bindValue(":pwd", QString::fromStdString(password));

    bool success = false;
    if (query.exec() && query.next()) {
        user.uid = query.value("uid").toInt();
        user.account = query.value("account").toString().toStdString();
        user.password = query.value("password").toString().toStdString();
        user.nickname = query.value("nickname").toString().toStdString();
        user.coins = query.value("coins").toInt();
        user.max_score = query.value("max_score").toInt();
        int lvl = query.value("max_level").toInt();
        user.max_level = (lvl < 1) ? 1 : lvl;
        user.item_bomb = query.value("item_bomb").toInt();
        user.item_reset = query.value("item_reset").toInt();
        user.item_freeze = query.value("item_freeze").toInt();
        success = true;
    }
    db.close();
    return success;
}

bool UserDao::registerUser(const std::string& account, const std::string& password, const std::string& nickname) {
    QSqlDatabase db = getDBConnection();
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT uid FROM users WHERE account = :acc");
    query.bindValue(":acc", QString::fromStdString(account));
    if (query.exec() && query.next()) {
        db.close();
        return false; // 账号已存在
    }

    query.prepare("INSERT INTO users (account, password, nickname, max_level, coins) VALUES (:acc, :pwd, :nick, 1, 500)");
    query.bindValue(":acc", QString::fromStdString(account));
    query.bindValue(":pwd", QString::fromStdString(password));
    query.bindValue(":nick", QString::fromStdString(nickname));

    bool success = query.exec();
    db.close();
    return success;
}

bool UserDao::getUserById(int uid, User& user) {
    QSqlDatabase db = getDBConnection();
    if (!db.open()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE uid = :uid");
    query.bindValue(":uid", uid);

    bool success = false;
    if (query.exec() && query.next()) {
        user.uid = uid;
        user.account = query.value("account").toString().toStdString();
        user.password = query.value("password").toString().toStdString();
        user.nickname = query.value("nickname").toString().toStdString();
        user.coins = query.value("coins").toInt();
        user.max_score = query.value("max_score").toInt();
        int lvl = query.value("max_level").toInt();
        user.max_level = (lvl < 1) ? 1 : lvl;
        user.item_bomb = query.value("item_bomb").toInt();
        user.item_reset = query.value("item_reset").toInt();
        user.item_freeze = query.value("item_freeze").toInt();
        success = true;
    }
    db.close();
    return success;
}

bool UserDao::updateAsset(int uid, const std::string& type, int delta) {
    if (uid <= 0) return false;

    QString connName = QUuid::createUuid().toString();
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("game.db");
        if (db.open()) {
            QSqlQuery q(db);

            // 如果是扣钱/扣道具，先查够不够
            if (delta < 0) {
                QString sql = QString("SELECT %1 FROM users WHERE uid = :uid").arg(QString::fromStdString(type));
                q.prepare(sql);
                q.bindValue(":uid", uid);
                if (q.exec() && q.next()) {
                    if (q.value(0).toInt() + delta < 0) {
                        // 余额不足
                        db.close();
                        QSqlDatabase::removeDatabase(connName);
                        return false;
                    }
                } else {
                    // 查不到用户
                    db.close();
                    QSqlDatabase::removeDatabase(connName);
                    return false;
                }
            }

            // 执行更新
            QString sql = QString("UPDATE users SET %1 = %1 + :delta WHERE uid = :uid").arg(QString::fromStdString(type));
            q.prepare(sql);
            q.bindValue(":delta", delta);
            q.bindValue(":uid", uid);
            success = q.exec();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return success;
}

bool UserDao::updateMaxScore(int uid, int current_score) {
    if (uid <= 0) return false;
    bool isNewRecord = false;

    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("game.db");
        if (db.open()) {
            QSqlQuery q(db);
            // 只在当前分更高时更新
            q.prepare("UPDATE users SET max_score = :score WHERE uid = :uid AND max_score < :score");
            q.bindValue(":score", current_score);
            q.bindValue(":uid", uid);
            if (q.exec()) {
                if (q.numRowsAffected() > 0) isNewRecord = true;
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return isNewRecord;
}

bool UserDao::updateMaxLevel(int uid, int level_passed) {
    if (uid <= 0) return false;
    bool unlocked = false;

    QString connName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("game.db");
        if (db.open()) {
            QSqlQuery q(db);
            // 查当前最大关卡
            q.prepare("SELECT max_level FROM users WHERE uid = :uid");
            q.bindValue(":uid", uid);
            if (q.exec() && q.next()) {
                int currentMax = q.value(0).toInt();
                if (currentMax < 1) currentMax = 1;

                // 只有刚好通关当前最大关卡时，才解锁下一关
                if (level_passed == currentMax) {
                    QSqlQuery updateQ(db);
                    // 通关奖励 100 金币 + 解锁
                    updateQ.prepare("UPDATE users SET max_level = max_level + 1, coins = coins + 100 WHERE uid = :uid");
                    updateQ.bindValue(":uid", uid);
                    if (updateQ.exec()) unlocked = true;
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return unlocked;
}

std::string UserDao::getNicknameFromDB(int uid) {
    if (uid == 0) return "Guest";

    std::string nick = "Player";
    QString connName = QUuid::createUuid().toString();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("game.db");
        if (db.open()) {
            QSqlQuery q(db);
            q.prepare("SELECT nickname FROM users WHERE uid = :uid");
            q.bindValue(":uid", uid);
            if (q.exec() && q.next()) {
                nick = q.value(0).toString().toStdString();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return nick;
}

json UserDao::getLeaderboard() {
    json list = json::array();
    QString connName = QUuid::createUuid().toString();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName("game.db");
        if (db.open()) {
            QSqlQuery q(db);
            q.exec("SELECT nickname, max_score FROM users ORDER BY max_score DESC LIMIT 10");
            while (q.next()) {
                list.push_back({
                    {"nickname", q.value("nickname").toString().toStdString()},
                    {"score", q.value("max_score").toInt()}
                });
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return list;
}