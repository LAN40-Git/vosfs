#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QObject>
#include <kosio/core.hpp>
#include "vosfs/ui/auth_client.hpp"

void kosio_thread(QQmlApplicationEngine* engine) {
    kosio::runtime::CurrentThreadBuilder::default_create().block_on(
        [engine]() -> kosio::async::Task<void> {
            auto* auth_client = new vosfs::ui::AuthClient("127.0.0.1", 9000);

            QMetaObject::invokeMethod(
                engine->rootContext(),
                [ctx = engine->rootContext(), auth_client]() {
                    ctx->setContextProperty("AuthClient", auth_client);
                },
                Qt::BlockingQueuedConnection
            );

            co_await kosio::signal::ctrl_c();
        }()
    );
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    QThread* thread = QThread::create(&kosio_thread, &engine);
    thread->start();
    engine.loadFromModule("main", "Main");
    return app.exec();
}