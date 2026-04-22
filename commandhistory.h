/**
 * @file commandhistory.h
 * @brief Histórico de comandos enviados, com persistência.
 */

#pragma once

#include <QObject>
#include <QStringList>
#include <QCompleter>

/**
 * @brief Mantém lista de comandos recentes e fornece auto-complete.
 */
class CommandHistory : public QObject {
    Q_OBJECT
public:
    explicit CommandHistory(QObject *parent = nullptr);

    /**
     * @brief Adiciona um comando ao histórico.
     * @param cmd Comando executado.
     */
    void addCommand(const QString& cmd);

    /**
     * @brief Retorna a lista de comandos recentes.
     */
    QStringList commands() const;

    /**
     * @brief Cria um QCompleter para um widget de edição.
     * @param parent Widget pai.
     * @return QCompleter configurado.
     */
    QCompleter* createCompleter(QObject *parent) const;

    /**
     * @brief Carrega histórico do disco.
     */
    void load();

    /**
     * @brief Salva histórico em disco.
     */
    void save();

private:
    QStringList history_;
    QString filePath_;
    static const int MAX_HISTORY = 100;
};