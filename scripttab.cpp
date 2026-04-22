#include "scripttab.h"
#include "automation/scriptengine.h"
#include "core/appsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

ScriptTab::ScriptTab(ScriptEngine *engine, QWidget *parent)
    : QWidget(parent)
    , scriptEngine(engine) {
    setupUi();

    connect(scriptEngine, &ScriptEngine::scriptOutput,
            this, &ScriptTab::onScriptOutput);
    connect(scriptEngine, &ScriptEngine::scriptFinished,
            this, &ScriptTab::onScriptFinished);
}

void ScriptTab::setupUi() {
    auto *layout = new QVBoxLayout(this);

    scriptEditor = new QTextEdit();
    scriptEditor->setPlaceholderText(tr(
        "Digite seu script JavaScript aqui...\n"
        "Exemplo:\n"
        "gpib.log('Iniciando');\n"
        "var resp = gpib.consultar(5, 0, '*IDN?');\n"
        "gpib.log('IDN: ' + resp);"
    ));

    auto *btnLayout = new QHBoxLayout();
    btnExecute = new QPushButton(tr("Executar"));
    btnLoad = new QPushButton(tr("Carregar Arquivo..."));
    btnLayout->addWidget(btnExecute);
    btnLayout->addWidget(btnLoad);
    btnLayout->addStretch();

    outputConsole = new QTextEdit();
    outputConsole->setReadOnly(true);
    outputConsole->setStyleSheet("background-color: #f0f0f0;");

    layout->addWidget(new QLabel(tr("Editor de Script:")));
    layout->addWidget(scriptEditor);
    layout->addLayout(btnLayout);
    layout->addWidget(new QLabel(tr("Saída:")));
    layout->addWidget(outputConsole);

    connect(btnExecute, &QPushButton::clicked, this, &ScriptTab::onExecuteScript);
    connect(btnLoad, &QPushButton::clicked, this, &ScriptTab::onLoadScript);
}

void ScriptTab::onExecuteScript() {
    QString script = scriptEditor->toPlainText();
    if (script.trimmed().isEmpty()) {
        outputConsole->append(tr("<font color='orange'>Script vazio.</font>"));
        return;
    }
    outputConsole->clear();
    outputConsole->append(tr("<i>Executando script...</i>"));
    scriptEngine->executarScriptAsync(script);
}

void ScriptTab::onLoadScript() {
    QString path = QFileDialog::getOpenFileName(this, tr("Carregar Script"), "",
                                                tr("Scripts (*.js);;Todos (*.*)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível abrir o arquivo."));
        return;
    }
    scriptEditor->setPlainText(file.readAll());
    AppSettings::instance().setUltimoScript(path);
}

void ScriptTab::onScriptOutput(const QString& text) {
    outputConsole->append(text);
}

void ScriptTab::onScriptFinished() {
    outputConsole->append(tr("<i>Script finalizado.</i>"));
}