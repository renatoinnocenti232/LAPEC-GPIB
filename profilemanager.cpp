#include "profilemanager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include "logger.h"

ProfileManager::ProfileManager(QObject *parent) : QObject(parent) {
    profilesDir_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
    QDir().mkpath(profilesDir_);
}

QString ProfileManager::getProfilePath(const QString& name) const {
    return profilesDir_ + "/" + name + ".json";
}

bool ProfileManager::saveProfile(const QString& name, const QVariantMap& settings) {
    QJsonObject obj = QJsonObject::fromVariantMap(settings);
    QJsonDocument doc(obj);
    QFile file(getProfilePath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        Logger::instance().error("Falha ao salvar perfil: " + name);
        return false;
    }
    file.write(doc.toJson());
    return true;
}

QVariantMap ProfileManager::loadProfile(const QString& name) const {
    QFile file(getProfilePath(name));
    if (!file.open(QIODevice::ReadOnly)) {
        Logger::instance().error("Falha ao carregar perfil: " + name);
        return {};
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object().toVariantMap();
}

QStringList ProfileManager::listProfiles() const {
    QDir dir(profilesDir_);
    QStringList filters("*.json");
    QStringList files = dir.entryList(filters, QDir::Files);
    QStringList names;
    for (const QString& f : files) {
        names.append(f.left(f.length() - 5)); // remove .json
    }
    return names;
}

bool ProfileManager::deleteProfile(const QString& name) {
    return QFile::remove(getProfilePath(name));
}