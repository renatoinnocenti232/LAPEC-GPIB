/**
 * @file profilemanager.h
 * @brief Gerenciamento de perfis de configuração salvos.
 */

#pragma once

#include <QObject>
#include <QVariantMap>
#include <QStringList>

/**
 * @brief Salva e carrega perfis de configuração (timeout, EOS, etc.).
 */
class ProfileManager : public QObject {
    Q_OBJECT
public:
    explicit ProfileManager(QObject *parent = nullptr);

    /**
     * @brief Salva um perfil com o nome especificado.
     * @param name Nome do perfil.
     * @param settings Mapa de configurações.
     * @return true se bem sucedido.
     */
    bool saveProfile(const QString& name, const QVariantMap& settings);

    /**
     * @brief Carrega um perfil.
     * @param name Nome do perfil.
     * @return Mapa de configurações, vazio se falhar.
     */
    QVariantMap loadProfile(const QString& name) const;

    /**
     * @brief Lista os perfis disponíveis.
     */
    QStringList listProfiles() const;

    /**
     * @brief Remove um perfil.
     */
    bool deleteProfile(const QString& name);

private:
    QString profilesDir_;
    QString getProfilePath(const QString& name) const;
};