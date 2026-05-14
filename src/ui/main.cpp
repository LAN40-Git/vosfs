#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QObject>
#include <kosio/core.hpp>
#include "vosfs/ui/client.hpp"

auto main(int argc, char *argv[]) -> int {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    auto signal_brige = vosfs::ui::SignalBrige{};

    vosfs::ui::ConfigBuilder::options()
        .set_auth_host("127.0.0.1")
        .set_auth_port(9000)
        .add_raft_node(1, "127.0.0.1", 8080)
        .add_raft_node(2, "127.0.0.1", 8081)
        .add_data_node(1, "127.0.0.1", 7070)
        .add_data_node(2, "127.0.0.1", 7071)
        .add_data_node(3, "127.0.0.1", 7072)
        .build().to_json("config.json");

    auto has_vosfs_client = vosfs::ui::VosfsClient::create("config.json", signal_brige);
    if (!has_vosfs_client) {
        LOG_ERROR("{}", has_vosfs_client.error());
        return -1;
    }
    auto vosfs_client = std::move(has_vosfs_client.value());
    auto* vosfs_thread = new QThread;
    vosfs_thread->setParent(&app);

    vosfs_client->moveToThread(vosfs_thread);
    engine.rootContext()->setContextProperty("SignalBrige", &signal_brige);
    engine.rootContext()->setContextProperty("VosfsClient", vosfs_client.get());

    QObject::connect(vosfs_thread, &QThread::started, vosfs_client.get(), &vosfs::ui::VosfsClient::run);
    // QObject::connect(vosfs_thread, &QThread::finished, &auth_client, &vosfs::ui::VosfsClient::deleteLater);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&](){
        vosfs_client->shutdown();
        vosfs_thread->quit();
        vosfs_thread->wait();
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    vosfs_thread->start();

    engine.loadFromModule("main", "Main");
    return QGuiApplication::exec();
}