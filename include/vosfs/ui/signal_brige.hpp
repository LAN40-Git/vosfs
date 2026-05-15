#pragma once
#include <QObject>

namespace vosfs::ui {
class SignalBrige : public QObject
{
    Q_OBJECT
public:
    explicit SignalBrige(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void appendLog(QString msg);
    void registerFinished(bool success, QString msg);
    void deleteFinished(bool success, QString msg);
    void loginFinished(bool success, QString msg);
    void listDirFinished(QString path, QVariantList dir_entries);
    void makeDirFinished();
    void uploadFileFinished();
};
} // namespace vosfs::ui