/**
 * @file scripttab.h
 * @brief Aba para edição e execução de scripts JavaScript.
 */

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class ScriptEngine;

class ScriptTab : public QWidget {
    Q_OBJECT

public:
    explicit ScriptTab(ScriptEngine *engine, QWidget *parent = nullptr);

private slots:
    void onExecuteScript();
    void onLoadScript();
    void onScriptOutput(const QString& text);
    void onScriptFinished();

private:
    void setupUi();

    ScriptEngine *scriptEngine;
    QTextEdit *scriptEditor;
    QTextEdit *outputConsole;
    QPushButton *btnExecute;
    QPushButton *btnLoad;
};