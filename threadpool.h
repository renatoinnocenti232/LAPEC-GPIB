/**
 * @file threadpool.h
 * @brief Pool de threads global para operações assíncronas.
 */

#pragma once

#include <QObject>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

/**
 * @brief Singleton que fornece um QThreadPool compartilhado.
 * 
 * Evita a criação excessiva de threads e permite limitar concorrência.
 */
class GpibThreadPool : public QObject {
    Q_OBJECT
public:
    static GpibThreadPool& instance();

    /**
     * @brief Executa uma função no pool de threads.
     * @param f Função a executar.
     * @return QFuture com o resultado da função.
     */
    template<typename Func>
    auto run(Func f) -> QFuture<decltype(f())> {
        return QtConcurrent::run(&pool_, f);
    }

    void setMaxThreadCount(int n) { pool_.setMaxThreadCount(n); }
    int maxThreadCount() const { return pool_.maxThreadCount(); }

private:
    GpibThreadPool() { pool_.setMaxThreadCount(QThread::idealThreadCount()); }
    QThreadPool pool_;
};