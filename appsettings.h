#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include "gpib_config.h"

class AppSettings {
public:
    static AppSettings& instance();

    // Configurações de comunicação
    Gpib::Timeout timeoutPadrao() const;
    void setTimeoutPadrao(Gpib::Timeout tmo);

    uint8_t eosChar() const;
    void setEosChar(uint8_t c);
    bool eosTermLeitura() const;
    void setEosTermLeitura(bool enable);
    bool eosEnviarEOI() const;
    void setEosEnviarEOI(bool enable);
    bool eosCmp8bits() const;
    void setEosCmp8bits(bool enable);

    int ultimoEndereco() const;
    void setUltimoEndereco(int addr);
    int ultimaPlaca() const;
    void setUltimaPlaca(int board);

    // Preferências gerais
    bool autoConectar() const;
    void setAutoConectar(bool enable);
    int intervaloLeituraMs() const;
    void setIntervaloLeituraMs(int ms);
    bool exportarCSVComCabecalho() const;
    void setExportarCSVComCabecalho(bool enable);

    // Configurações da API
    bool apiHabilitada() const;
    void setApiHabilitada(bool enable);
    bool apiApenasLocalhost() const;
    void setApiApenasLocalhost(bool enable);
    QString apiKey() const;
    void setApiKey(const QString& key);
    quint16 apiPortaWebSocket() const;
    void setApiPortaWebSocket(quint16 port);
    quint16 apiPortaRest() const;
    void setApiPortaRest(quint16 port);

    // Caminhos recentes
    QString ultimoScript() const;
    void setUltimoScript(const QString& path);

    // MELHORIA: sincronização explícita para evitar escritas excessivas
    void sync();

private:
    AppSettings();
    QSettings settings_;
};

#endif // APPSETTINGS_H