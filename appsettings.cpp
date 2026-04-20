#include "appsettings.h"

AppSettings& AppSettings::instance() {
    static AppSettings inst;
    return inst;
}

AppSettings::AppSettings() : settings_("GpibMaestro", "Config") {}

Gpib::Timeout AppSettings::timeoutPadrao() const {
    return static_cast<Gpib::Timeout>(settings_.value("timeout", 13).toInt());
}
void AppSettings::setTimeoutPadrao(Gpib::Timeout tmo) {
    settings_.setValue("timeout", static_cast<int>(tmo));
}

uint8_t AppSettings::eosChar() const {
    return static_cast<uint8_t>(settings_.value("eosChar", 10).toInt());
}
void AppSettings::setEosChar(uint8_t c) {
    settings_.setValue("eosChar", c);
}

bool AppSettings::eosTermLeitura() const {
    return settings_.value("eosTermLeitura", true).toBool();
}
void AppSettings::setEosTermLeitura(bool enable) {
    settings_.setValue("eosTermLeitura", enable);
}

bool AppSettings::eosEnviarEOI() const {
    return settings_.value("eosEnviarEOI", false).toBool();
}
void AppSettings::setEosEnviarEOI(bool enable) {
    settings_.setValue("eosEnviarEOI", enable);
}

bool AppSettings::eosCmp8bits() const {
    return settings_.value("eosCmp8bits", false).toBool();
}
void AppSettings::setEosCmp8bits(bool enable) {
    settings_.setValue("eosCmp8bits", enable);
}

int AppSettings::ultimoEndereco() const {
    return settings_.value("ultimoEndereco", -1).toInt();
}
void AppSettings::setUltimoEndereco(int addr) {
    settings_.setValue("ultimoEndereco", addr);
}

bool AppSettings::autoConectar() const {
    return settings_.value("autoConectar", true).toBool();
}
void AppSettings::setAutoConectar(bool enable) {
    settings_.setValue("autoConectar", enable);
}

int AppSettings::intervaloLeituraMs() const {
    return settings_.value("intervaloLeituraMs", 1000).toInt();
}
void AppSettings::setIntervaloLeituraMs(int ms) {
    settings_.setValue("intervaloLeituraMs", ms);
}