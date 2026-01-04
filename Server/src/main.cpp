#define ASIO_STANDALONE
#include "crow_all.h"
#include "controllers/AuthController.h"
#include "controllers/GameController.h"
#include "utils/WebPage.h" 
#include <QCoreApplication>

// CORS 中间件
struct CORSMiddleware {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // No-op
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

int main(int argc, char** argv) {
    QCoreApplication qtApp(argc, argv);

    crow::App<CORSMiddleware> app;

    // 全局 OPTIONS 路由
    CROW_ROUTE(app, "/<path>")
        .methods("OPTIONS"_method)
        ([](const crow::request&, crow::response& res, std::string) {
            res.code = 200;
            res.end();
        });

    // 读取并返回html
    CROW_ROUTE(app, "/")
    ([](const crow::request&, crow::response& res) {
        std::string content = WebPage::readFile("index.html");

        if (content.empty()) {
            res.code = 404;
            res.write("Error: static/index.html not found. Please check your CMake copy configuration.");
        } else {
            res.set_header("Content-Type", "text/html; charset=utf-8");
            res.write(content);
        }
        res.end();
    });

    // 静态资源路由
    CROW_ROUTE(app, "/<string>")
    ([](const crow::request&, crow::response& res, std::string filename) {
        std::string content = WebPage::readFile(filename);

        if (content.empty()) {
            res.code = 404;
            res.write("File not found");
        } else {
            res.set_header("Content-Type", WebPage::getMimeType(filename));
            res.write(content);
        }
        res.end();
    });


    AuthController authController;  //用户操作核心逻辑
    GameController gameController;  //游戏操作核心逻辑

    // 注册业务逻辑路由
    authController.registerRoutes(app);
    gameController.registerRoutes(app);

    // 启动服务
    app.port(8000).multithreaded().run();

    return 0;
}