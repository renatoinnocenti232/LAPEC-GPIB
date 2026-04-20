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

    // Outras preferências
    bool autoConectar() const;
    void setAutoConectar(bool enable);
    int intervaloLeituraMs() const;
    void setIntervaloLeituraMs(int ms);

private:
    AppSettings();
    QSettings settings_;
};

#endif