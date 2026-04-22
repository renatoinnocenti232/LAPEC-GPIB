#include <QtTest/QtTest>
#include "gpib_config.h"

// Mock simples para testes (não depende da biblioteca real)
class MockGpib {
public:
    static int ibdev(int board, int pad, int sad, int tmo, int eot, int eos) {
        return pad + board * 100; // handle fictício
    }
    static int ibonl(int handle, int v) { return 0; }
    static int ibwrt(int handle, const char* buf, long cnt) { return cnt; }
    static int ibrd(int handle, char* buf, long cnt) { 
        strcpy(buf, "TEST");
        return 4;
    }
    static int ibclr(int handle) { return 0; }
    static int ibeos(int handle, int v) { return 0; }
    // ... outros métodos mockados conforme necessário
};

// Redefine as chamadas para usar o mock (apenas para teste)
#define ibdev MockGpib::ibdev
#define ibonl MockGpib::ibonl
#define ibwrt MockGpib::ibwrt
#define ibrd  MockGpib::ibrd
#define ibclr MockGpib::ibclr
#define ibeos MockGpib::ibeos

class TestGpibConfig : public QObject {
    Q_OBJECT

private slots:
    void testCriacaoInstrumento() {
        try {
            Gpib::InstrumentoMestre instr(5, 0, Gpib::Timeout::T10s, true);
            QCOMPARE(instr.endereco(), 5);
            QCOMPARE(instr.placa(), 0);
        } catch (const std::exception& e) {
            QFAIL("Exceção inesperada");
        }
    }

    void testEnviarComando() {
        Gpib::InstrumentoMestre instr(5);
        QVERIFY_NO_THROW(instr.enviar("*IDN?"));
    }

    void testLerResposta() {
        Gpib::InstrumentoMestre instr(5);
        std::string resp = instr.ler(100);
        QCOMPARE(resp, "TEST");
    }

    void testTimeoutEnum() {
        QCOMPARE(static_cast<int>(Gpib::Timeout::T10s), 13);
        QCOMPARE(static_cast<int>(Gpib::Timeout::T1s), 11);
    }
};

QTEST_MAIN(TestGpibConfig)
#include "test_gpibconfig.moc"