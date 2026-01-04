#include <QApplication>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QUrl>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);

    QWebEngineView view;
    view.setWindowTitle("Gemini Game Universe - Client");
    view.resize(800, 800);

    view.page()->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING_PORT", "9000");

    QUrl serverUrl("http://120.46.159.115:8000/");

    qDebug() << "正在连接游戏服务器:" << serverUrl;
    view.load(serverUrl);

    view.show();
    return app.exec();
}