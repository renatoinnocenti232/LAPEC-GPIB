/**
 * @file gpib_config.h
 * @brief Interface de alto nível para comunicação GPIB (NI-488.2).
 * 
 * Encapsula as chamadas à biblioteca gpib (Linux) ou NI-488.2 (Windows).
 * Fornece exceções tipadas e gerenciamento de múltiplos instrumentos.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <map>

// Inclusão do header da NI-488.2 (deve estar no path)
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "ni4882.h"  // ou "gpib.h" dependendo da instalação

namespace Gpib {

    // ========== Exceções Específicas ==========

    /**
     * @brief Exceção base para erros GPIB.
     */
    class GpibException : public std::runtime_error {
    public:
        explicit GpibException(const std::string& msg) : std::runtime_error(msg) {}
    };

    /**
     * @brief Exceção para falhas de conexão.
     */
    class ConnectionException : public GpibException {
    public:
        explicit ConnectionException(const std::string& msg) : GpibException(msg) {}
    };

    /**
     * @brief Exceção para timeout em operações.
     */
    class TimeoutException : public GpibException {
    public:
        explicit TimeoutException(const std::string& msg) : GpibException(msg) {}
    };

    /**
     * @brief Exceção para erros de comunicação gerais.
     */
    class CommunicationException : public GpibException {
    public:
        explicit CommunicationException(const std::string& msg) : GpibException(msg) {}
    };

    // ========== Enums e Configurações ==========

    /**
     * @brief Valores de timeout suportados pela API GPIB.
     */
    enum class Timeout : int {
        None   = 0,   T10us  = 1,   T30us  = 2,   T100us = 3,   T300us = 4,
        T1ms   = 5,   T3ms   = 6,   T10ms  = 7,   T30ms  = 8,   T100ms = 9,
        T300ms = 10,  T1s    = 11,  T3s    = 12,  T10s   = 13,  T30s   = 14,
        T100s  = 15,  T300s  = 16,  T1000s = 17
    };

    /**
     * @brief Opções de configuração para ibconfig().
     */
    enum class ConfigOption : unsigned int {
        PAD       = 0x0001, ///< Endereço primário
        SAD       = 0x0002, ///< Endereço secundário
        TMO       = 0x0003, ///< Timeout
        EOT       = 0x0004, ///< Enviar EOI ao final
        EOSrd     = 0x0005, ///< Terminação de leitura
        EOSwrt    = 0x0006, ///< Terminação de escrita
        EOScmp    = 0x0007, ///< Modo de comparação EOS
        EOSchar   = 0x0008, ///< Caractere EOS
        SendLLO   = 0x0014, ///< Enviar LLO
        UnAddr    = 0x0017  ///< Desendereçar após operação
    };

    // ========== Classe Principal: InstrumentoMestre ==========

    /**
     * @brief Representa um instrumento GPIB conectado.
     * 
     * Fornece métodos para enviar comandos, ler respostas e configurar
     * parâmetros de comunicação. Thread-safe.
     */
    class InstrumentoMestre {
    private:
        int handle_{-1};               ///< Handle da sessão GPIB
        int endereco_{-1};             ///< Endereço primário
        int placa_{0};                 ///< Número da placa GPIB
        mutable std::mutex mtx_;       ///< Mutex para operações thread-safe

        /**
         * @brief Verifica ibsta e lança exceção apropriada em caso de erro.
         * @param acao Descrição da operação para a mensagem de erro.
         */
        void validar(const std::string& acao) const;

    public:
        /**
         * @brief Construtor. Abre uma sessão com o instrumento.
         * @param pad Endereço primário (0-30).
         * @param placa Número da placa GPIB (padrão 0).
         * @param tmo Timeout inicial.
         * @param resetDevice Se true, envia Device Clear após conexão.
         * @throw ConnectionException se não for possível conectar.
         */
        explicit InstrumentoMestre(int pad, int placa = 0,
                                   Timeout tmo = Timeout::T10s,
                                   bool resetDevice = true);

        /**
         * @brief Destrutor. Fecha a sessão GPIB.
         */
        ~InstrumentoMestre();

        // ----- Operações de Barramento -----

        /**
         * @brief Varre o barramento em busca de dispositivos.
         * @param placa Número da placa.
         * @param timeoutMs Timeout para a operação FindLstn (0 = usa padrão).
         * @return Vetor com os endereços dos dispositivos encontrados.
         */
        static std::vector<short> VarrerBarramento(int placa = 0, int timeoutMs = 5000);

        // ----- Comunicação Básica -----

        /**
         * @brief Envia um comando (sem esperar resposta).
         * @param cmd String a ser enviada.
         * @return Referência para o próprio objeto (encadeamento).
         */
        InstrumentoMestre& enviar(const std::string& cmd);

        /**
         * @brief Lê dados do instrumento.
         * @param tamanho Número máximo de bytes a ler (padrão 2048).
         * @return String com os dados lidos.
         */
        std::string ler(size_t tamanho = 2048);

        /**
         * @brief Envia um comando e lê a resposta.
         * @param cmd Comando a enviar.
         * @return Resposta do instrumento.
         */
        std::string query(const std::string& cmd);

        // ----- Recursos Avançados -----

        /**
         * @brief Executa Serial Poll e retorna o Status Byte.
         * @return Status Byte.
         */
        uint8_t lerStatusByte();

        /**
         * @brief Eventos que podem ser aguardados com ibwait.
         */
        enum class Evento : unsigned short {
            SRQ   = 0x0800, ///< Service Request
            TIMO  = 0x4000, ///< Timeout
            CMPL  = 0x0100  ///< Operação completada
        };

        /**
         * @brief Aguarda um ou mais eventos.
         * @param mascara Combinação de Evento.
         * @return true se o evento ocorreu.
         */
        bool esperarEvento(unsigned short mascara);

        /**
         * @brief Aguarda até que o instrumento gere SRQ.
         * @return true se SRQ ocorreu, false em timeout.
         */
        bool esperarSRQ();

        /**
         * @brief Envia dados para um endereço específico (modo controlador).
         * @param placa Número da placa.
         * @param endereco Endereço do listener.
         * @param dados Dados a enviar.
         * @param eoi Se true, afirma EOI com o último byte.
         */
        static void enviarPara(int placa, int endereco, const std::string& dados, bool eoi = true);

        /**
         * @brief Recebe dados de um endereço específico (modo controlador).
         * @param placa Número da placa.
         * @param endereco Endereço do talker.
         * @param maxBytes Número máximo de bytes a ler.
         * @param terminador Caractere de terminação (0 = usa EOI).
         * @return String com os dados recebidos.
         */
        static std::string receberDe(int placa, int endereco, size_t maxBytes, char terminador = 0);

        /**
         * @brief Configura um dispositivo para Parallel Poll.
         * @param placa Número da placa.
         * @param endereco Endereço do dispositivo.
         * @param dataLine Linha DIO a ser usada (1-8).
         * @param sense true = resposta ativa em nível alto.
         */
        static void configurarParallelPoll(int placa, int endereco, int dataLine, bool sense);

        /**
         * @brief Executa um Parallel Poll.
         * @param placa Número da placa.
         * @return Byte de resposta.
         */
        static uint8_t executarParallelPoll(int placa);

        /**
         * @brief Desconfigura Parallel Poll de uma lista de dispositivos.
         * @param placa Número da placa.
         * @param enderecos Lista de endereços a desconfigurar.
         */
        static void desconfigurarParallelPoll(int placa, const std::vector<short>& enderecos);

        /**
         * @brief Passa o controle para outro dispositivo (torna-se Controller-In-Charge).
         * @param placa Número da placa.
         * @param endereco Endereço do novo CIC.
         */
        static void passarControle(int placa, int endereco);

        /**
         * @brief Define a configuração de EOS/EOI.
         * @param eosChar Caractere de fim de string.
         * @param terminarLeitura Se true, leitura termina ao encontrar EOS.
         * @param enviarEOI Se true, EOI é enviado com o EOS.
         * @param cmp8bits Se true, comparação de 8 bits (ignora bit 7).
         */
        void definirEOS(uint8_t eosChar, bool terminarLeitura = true,
                        bool enviarEOI = false, bool cmp8bits = false);

        /**
         * @brief Habilita/desabilita o envio de EOI ao final da escrita.
         * @param habilitar true para habilitar.
         */
        void habilitarEOI(bool habilitar);

        /**
         * @brief Altera o timeout da sessão.
         * @param tmo Novo valor de timeout.
         */
        void definirTimeout(Timeout tmo);

        /**
         * @brief Configura parâmetros avançados via ibconfig.
         * @param opcao Opção a configurar.
         * @param valor Novo valor.
         * @return Valor anterior da opção.
         */
        int configurar(ConfigOption opcao, unsigned int valor);

        /**
         * @brief Habilita/desabilita transferências DMA.
         * @param habilitar true para habilitar.
         */
        void habilitarDMA(bool habilitar);

        // Getters
        int endereco() const { return endereco_; }
        int placa() const { return placa_; }
    };

    /**
     * @brief Operador bit-a-bit para combinar Evento.
     */
    inline unsigned short operator|(InstrumentoMestre::Evento a, InstrumentoMestre::Evento b) {
        return static_cast<unsigned short>(a) | static_cast<unsigned short>(b);
    }

    // ========== Gerenciador de Múltiplos Instrumentos ==========

    /**
     * @brief Singleton que gerencia instâncias de InstrumentoMestre.
     * 
     * Mantém um cache de instrumentos conectados, evitando múltiplas conexões
     * para o mesmo endereço. Thread-safe.
     */
    class GpibManager {
    public:
        static GpibManager& instance();

        /**
         * @brief Obtém um instrumento (cria se não existir).
         * @param endereco Endereço GPIB.
         * @param placa Número da placa (padrão 0).
         * @param reset Se true, envia Device Clear ao criar nova conexão.
         * @return shared_ptr para o instrumento.
         */
        std::shared_ptr<InstrumentoMestre> getInstrumento(int endereco, int placa = 0, bool reset = false);

        /**
         * @brief Remove um instrumento do cache.
         * @param endereco Endereço a remover.
         */
        void removerInstrumento(int endereco);

        /**
         * @brief Lista todos os endereços atualmente gerenciados.
         * @return Vetor de endereços.
         */
        std::vector<int> listarInstrumentosAtivos() const;

    private:
        GpibManager() = default;
        std::map<int, std::shared_ptr<InstrumentoMestre>> instrumentos_;
        mutable std::mutex mtx_;
    };

} // namespace Gpib