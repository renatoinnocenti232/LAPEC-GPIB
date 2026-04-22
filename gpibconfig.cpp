/**
 * @file gpib_config.cpp
 * @brief Implementação das classes de comunicação GPIB.
 */

#include "gpib_config.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>

namespace Gpib {

    // ---------- Função auxiliar para lançar exceções baseadas em ibsta/iberr ----------
    static void throwFromIbsta(const std::string& acao) {
        std::ostringstream oss;
        oss << "Erro em [" << acao << "] | Código: " << iberr;
        if (ibsta & TIMO) {
            throw TimeoutException(oss.str());
        } else if (ibsta & ERR) {
            if (iberr == EDVR || iberr == ENEB || iberr == EHDL)
                throw ConnectionException(oss.str());
            else
                throw CommunicationException(oss.str());
        }
    }

    // ---------- Construtor / Destrutor ----------
    InstrumentoMestre::InstrumentoMestre(int pad, int placa, Timeout tmo, bool resetDevice)
        : endereco_(pad), placa_(placa) {
        const int EOT_ENABLED  = 1;
        const int EOS_DISABLED = 0;
        
        handle_ = ibdev(placa, pad, 0, static_cast<int>(tmo), EOT_ENABLED, EOS_DISABLED);
        if (handle_ < 0) {
            throw ConnectionException("Falha ao abrir dispositivo GPIB. Verifique cabos e endereço.");
        }
        if (resetDevice) {
            ibclr(handle_);
            if (ibsta & ERR) throw CommunicationException("Falha ao enviar Device Clear.");
        }
    }

    InstrumentoMestre::~InstrumentoMestre() {
        if (handle_ >= 0) ibonl(handle_, 0);
    }

    void InstrumentoMestre::validar(const std::string& acao) const {
        if (ibsta & (ERR | TIMO)) {
            throwFromIbsta(acao);
        }
    }

    // ---------- Varredura de barramento ----------
    std::vector<short> InstrumentoMestre::VarrerBarramento(int placa, int timeoutMs) {
        // Configura timeout temporário para a varredura
        int oldTmo = ibtmo(placa, static_cast<int>(Timeout::T10s));
        if (timeoutMs > 0) {
            // Mapear ms para enum Timeout (aproximado)
            Timeout tmo;
            if (timeoutMs <= 1) tmo = Timeout::T1ms;
            else if (timeoutMs <= 3) tmo = Timeout::T3ms;
            else if (timeoutMs <= 10) tmo = Timeout::T10ms;
            else if (timeoutMs <= 30) tmo = Timeout::T30ms;
            else if (timeoutMs <= 100) tmo = Timeout::T100ms;
            else if (timeoutMs <= 300) tmo = Timeout::T300ms;
            else if (timeoutMs <= 1000) tmo = Timeout::T1s;
            else if (timeoutMs <= 3000) tmo = Timeout::T3s;
            else if (timeoutMs <= 10000) tmo = Timeout::T10s;
            else if (timeoutMs <= 30000) tmo = Timeout::T30s;
            else tmo = Timeout::T30s;
            ibtmo(placa, static_cast<int>(tmo));
        }

        short addr_list[32], results[32];
        for (short i = 0; i < 31; ++i) addr_list[i] = i + 1;
        addr_list[31] = -1;

        FindLstn(placa, addr_list, results, 31);
        if (ibsta & ERR) {
            ibtmo(placa, oldTmo); // restaura timeout
            throwFromIbsta("FindLstn");
        }
        ibtmo(placa, oldTmo); // restaura timeout

        std::vector<short> lista;
        for (int i = 0; results[i] != -1 && i < 31; ++i) {
            lista.push_back(results[i]);
        }
        return lista;
    }

    // ---------- Comunicação básica ----------
    InstrumentoMestre& InstrumentoMestre::enviar(const std::string& cmd) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibwrt(handle_, cmd.c_str(), static_cast<long>(cmd.length()));
        validar("Escrita");
        return *this;
    }

    std::string InstrumentoMestre::ler(size_t tamanho) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<char> buffer(tamanho + 1, 0);
        ibrd(handle_, buffer.data(), static_cast<long>(tamanho));
        validar("Leitura");
        std::string s(buffer.data(), ibcntl);
        // Remove caracteres de controle comuns
        s.erase(std::remove_if(s.begin(), s.end(), [](char c){ return c=='\r'||c=='\n'; }), s.end());
        return s;
    }

    std::string InstrumentoMestre::query(const std::string& cmd) {
        return enviar(cmd).ler();
    }

    // ---------- Serial Poll ----------
    uint8_t InstrumentoMestre::lerStatusByte() {
        std::lock_guard<std::mutex> lock(mtx_);
        char spr;
        ibrsp(handle_, &spr);
        validar("Serial Poll");
        return static_cast<uint8_t>(spr);
    }

    // ---------- Espera por eventos ----------
    bool InstrumentoMestre::esperarEvento(unsigned short mascara) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibwait(handle_, mascara);
        validar("ibwait");
        return (ibsta & mascara) != 0;
    }

    bool InstrumentoMestre::esperarSRQ() {
        return esperarEvento(static_cast<unsigned short>(Evento::SRQ));
    }

    // ---------- Send/Receive 488.2 ----------
    void InstrumentoMestre::enviarPara(int placa, int endereco, const std::string& dados, bool eoi) {
        int eotMode = eoi ? DABend : NULLend;
        Send(placa, endereco, const_cast<char*>(dados.c_str()), static_cast<long>(dados.size()), eotMode);
        if (ibsta & ERR) {
            std::ostringstream oss;
            oss << "Erro em Send para endereço " << endereco << " | Código: " << iberr;
            throw CommunicationException(oss.str());
        }
    }

    std::string InstrumentoMestre::receberDe(int placa, int endereco, size_t maxBytes, char terminador) {
        std::vector<char> buffer(maxBytes + 1, 0);
        int term = (terminador == 0) ? STOPend : static_cast<int>(terminador);
        Receive(placa, endereco, buffer.data(), static_cast<long>(maxBytes), term);
        if (ibsta & ERR) {
            std::ostringstream oss;
            oss << "Erro em Receive do endereço " << endereco << " | Código: " << iberr;
            throw CommunicationException(oss.str());
        }
        return std::string(buffer.data(), ibcntl);
    }

    // ---------- Parallel Poll ----------
    void InstrumentoMestre::configurarParallelPoll(int placa, int endereco, int dataLine, bool sense) {
        PPollConfig(placa, endereco, dataLine, sense ? 1 : 0);
        if (ibsta & ERR) {
            throwFromIbsta("PPollConfig");
        }
    }

    uint8_t InstrumentoMestre::executarParallelPoll(int placa) {
        short result;
        PPoll(placa, &result);
        if (ibsta & ERR) {
            throwFromIbsta("PPoll");
        }
        return static_cast<uint8_t>(result);
    }

    void InstrumentoMestre::desconfigurarParallelPoll(int placa, const std::vector<short>& enderecos) {
        std::vector<short> addrList = enderecos;
        addrList.push_back(NOADDR);
        PPollUnconfig(placa, addrList.data());
        if (ibsta & ERR) {
            throwFromIbsta("PPollUnconfig");
        }
    }

    // ---------- Pass Control ----------
    void InstrumentoMestre::passarControle(int placa, int endereco) {
        PassControl(placa, endereco);
        if (ibsta & ERR) {
            throwFromIbsta("PassControl");
        }
    }

    // ---------- Configuração de EOS / EOI ----------
    void InstrumentoMestre::definirEOS(uint8_t eosChar, bool terminarLeitura, bool enviarEOI, bool cmp8bits) {
        std::lock_guard<std::mutex> lock(mtx_);
        int modo = eosChar;
        if (terminarLeitura) modo |= REOS;
        if (enviarEOI)       modo |= XEOS;
        if (cmp8bits)        modo |= BIN;
        ibeos(handle_, modo);
        validar("ibeos");
    }

    void InstrumentoMestre::habilitarEOI(bool habilitar) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibeot(handle_, habilitar ? 1 : 0);
        validar("ibeot");
    }

    // ---------- Timeout dinâmico ----------
    void InstrumentoMestre::definirTimeout(Timeout tmo) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibtmo(handle_, static_cast<int>(tmo));
        validar("ibtmo");
    }

    // ---------- Configuração genérica e DMA ----------
    int InstrumentoMestre::configurar(ConfigOption opcao, unsigned int valor) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibconfig(handle_, static_cast<unsigned int>(opcao), valor);
        validar("ibconfig");
        return iberr;   // iberr contém o valor anterior (documentado)
    }

    void InstrumentoMestre::habilitarDMA(bool habilitar) {
        std::lock_guard<std::mutex> lock(mtx_);
        ibdma(handle_, habilitar ? 1 : 0);
        validar("ibdma");
    }

    // ========== GpibManager ==========
    GpibManager& GpibManager::instance() {
        static GpibManager mgr;
        return mgr;
    }

    std::shared_ptr<InstrumentoMestre> GpibManager::getInstrumento(int endereco, int placa, bool reset) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = instrumentos_.find(endereco);
        if (it != instrumentos_.end() && it->second) {
            return it->second;
        }
        auto novo = std::make_shared<InstrumentoMestre>(endereco, placa, Timeout::T10s, reset);
        instrumentos_[endereco] = novo;
        return novo;
    }

    void GpibManager::removerInstrumento(int endereco) {
        std::lock_guard<std::mutex> lock(mtx_);
        instrumentos_.erase(endereco);
    }

    std::vector<int> GpibManager::listarInstrumentosAtivos() const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<int> lista;
        for (const auto& par : instrumentos_) {
            lista.push_back(par.first);
        }
        return lista;
    }

} // namespace Gpib