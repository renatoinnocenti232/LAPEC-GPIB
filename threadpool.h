#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

class GpibThreadPool : public QObject {
    Q_OBJECT
public:
    static GpibThreadPool& instance();

    // Executa uma função em uma thread do pool e retorna QFuture
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

#endif // THREADPOOL_H