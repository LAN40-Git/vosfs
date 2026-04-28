#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <kosio/core.hpp>
#include "vosfs/ui/auth_client.hpp"

auto main_coro(int argc, char *argv[]) -> kosio::async::Task<void> {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    vosfs::ui::AuthClient auth_client{"127.0.0.1", 9000};
    engine.rootContext()->setContextProperty("AuthClient", &auth_client);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("main", "Main");
    QCoreApplication::exec();
    co_return;
}

int main(int argc, char *argv[]) {
    kosio::runtime::MultiThreadBuilder::default_create().block_on(main_coro(argc, argv));
}
