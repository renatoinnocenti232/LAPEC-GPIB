#ifndef GPIB_CONFIG_H
#define GPIB_CONFIG_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace Gpib {

    // ========== Exceções Específicas (1) ==========
    class GpibException : public std::runtime_error {
    public:
        explicit GpibException(const std::string& msg) : std::runtime_error(msg) {}
    };

    class ConnectionException : public GpibException {
    public:
        explicit ConnectionException(const std::string& msg) : GpibException(msg) {}
    };

    class TimeoutException : public GpibException {
    public:
        explicit TimeoutException(const std::string& msg) : GpibException(msg) {}
    };

    class CommunicationException : public GpibException {
    public:
        explicit CommunicationException(const std::string& msg) : GpibException(msg) {}
    };

    // ========== Enums e Configurações ==========
    enum class Timeout : int {
        None   = 0,   T10us  = 1,   T30us  = 2,   T100us = 3,   T300us = 4,
        T1ms   = 5,   T3ms   = 6,   T10ms  = 7,   T30ms  = 8,   T100ms = 9,
        T300ms = 10,  T1s    = 11,  T3s    = 12,  T10s   = 13,  T30s   = 14,
        T100s  = 15,  T300s  = 16,  T1000s = 17
    };

    enum class ConfigOption : unsigned int {
        PAD       = 0x0001, SAD       = 0x0002, TMO       = 0x0003,
        EOT       = 0x0004, EOSrd     = 0x0005, EOSwrt    = 0x0006,
        EOScmp    = 0x0007, EOSchar   = 0x0008, SendLLO   = 0x0014,
        UnAddr    = 0x0017
    };

    class InstrumentoMestre {
    private:
        int handle_{-1};
        int endereco_{-1};
        int placa_{0};
        mutable std::mutex mtx_;

        void validar(const std::string& acao) const;

    public:
        explicit InstrumentoMestre(int pad, int placa = 0,
                                   Timeout tmo = Timeout::T10s,
                                   bool resetDevice = true);
        ~InstrumentoMestre();

        // ----- Operações de Barramento -----
        static std::vector<short> VarrerBarramento(int placa = 0);

        // ----- Comunicação Básica -----
        InstrumentoMestre& enviar(const std::string& cmd);
        std::string ler(size_t tamanho = 2048);
        std::string query(const std::string& cmd);

        // ----- Recursos Avançados -----
        uint8_t lerStatusByte();

        enum class Evento : unsigned short {
            SRQ   = 0x0800, TIMO  = 0x4000, CMPL  = 0x0100
        };
        bool esperarEvento(unsigned short mascara);
        bool esperarSRQ();

        static void enviarPara(int placa, int endereco, const std::string& dados, bool eoi = true);
        static std::string receberDe(int placa, int endereco, size_t maxBytes, char terminador = 0);

        static void configurarParallelPoll(int placa, int endereco, int dataLine, bool sense);
        static uint8_t executarParallelPoll(int placa);
        static void desconfigurarParallelPoll(int placa, const std::vector<short>& enderecos);

        static void passarControle(int placa, int endereco);

        void definirEOS(uint8_t eosChar, bool terminarLeitura = true,
                        bool enviarEOI = false, bool cmp8bits = false);
        void habilitarEOI(bool habilitar);

        void definirTimeout(Timeout tmo);

        int configurar(ConfigOption opcao, unsigned int valor);
        void habilitarDMA(bool habilitar);

        // Getters para uso externo
        int endereco() const { return endereco_; }
        int placa() const { return placa_; }
    };

    inline unsigned short operator|(InstrumentoMestre::Evento a, InstrumentoMestre::Evento b) {
        return static_cast<unsigned short>(a) | static_cast<unsigned short>(b);
    }

    // ========== Gerenciador de Múltiplos Instrumentos (8) ==========
    class GpibManager {
    public:
        static GpibManager& instance();

        std::shared_ptr<InstrumentoMestre> getInstrumento(int endereco, int placa = 0, bool reset = false);
        void removerInstrumento(int endereco);
        std::vector<int> listarInstrumentosAtivos() const;

    private:
        GpibManager() = default;
        std::map<int, std::shared_ptr<InstrumentoMestre>> instrumentos_;
        mutable std::mutex mtx_;
    };

} // namespace Gpib

#endif