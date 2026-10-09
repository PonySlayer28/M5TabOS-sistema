// Parse de NMEA - LOGICA PURA.
//
// De proposito, este arquivo e o nmea.cpp nao incluem M5GFX, M5Unified nem
// nada de tela: so string e struct. Isso torna o parser testavel fora da
// placa, reaproveitavel em outro projeto (o Meshtastic vai querer algo
// parecido) e deixa obvio, olhando a pasta, o que e regra de negocio e o que e
// interface.

#pragma once

#include <stdint.h>
#include <stddef.h>

#define NMEA_MAX_LENGTH   128
// Com varias constelacoes ao mesmo tempo, 32 aperta. GPS(12) + GLONASS(10) +
// Galileo(10) + BeiDou visiveis passam disso com facilidade.
#define MAX_SATS_TRACKED  48

typedef enum {
    GPS, GLONASS, GALILEO, BEIDOU, QZSS, SBAS, UNKNOWN
} GnssType;

// Uma sentenca NMEA bruta e completa, ja com checksum validado.
typedef struct {
    char sentence[NMEA_MAX_LENGTH];
} NMEARawSentence_t;

// Dado de UM satelite, ja parseado - e isso que o skyplot consome.
typedef struct {
    GnssType type;
    uint8_t  prn;         // ID do satelite (1-32 GPS, etc)
    uint8_t  elevation;   // 0-90 graus (0 = horizonte, 90 = zenite)
    uint16_t azimuth;     // 0-359 graus (0 = Norte, 90 = Leste)
    uint8_t  snr;         // 0-99 dB (0 = nao capturado ainda)

    // O receptor SABE que este satelite existe, mas ainda nao tem posicao para
    // ele: no NMEA isso vem como campos de elevacao/azimute VAZIOS. Como
    // atoi("") devolve 0, sem esta flag todos eles cairiam no mesmo ponto do
    // grafico (elevacao 0 = horizonte, azimute 0 = norte) e ficariam empilhados.
    // Quem desenha deve pular os que tem tem_posicao == false.
    bool     tem_posicao;

    // QUANDO a posicao acima foi de fato atualizada (ms de uptime).
    //
    // Existe porque "tem_posicao" nao distingue observacao nova de eco antigo.
    // O parser mantem a ultima posicao conhecida quando a GSV chega sem
    // elevacao/azimute - o que e CERTO para a tela (evita o satelite piscar
    // entre existir e sumir) e ERRADO para o registro em arquivo.
    //
    // Sem isso, o app gravou 2047 linhas ao longo de 2h45m que
    // colapsaram em 23 posicoes: o receptor parou de mandar posicao, o parser
    // segurou a ultima, e o gravador escreveu o mesmo ponto todo minuto como se
    // fosse observacao nova. Quem GRAVA deve olhar esta idade; quem DESENHA
    // pode ignorar.
    uint32_t pos_ms;

    bool     valid;
} SatelliteInfo_t;

// Snapshot completo da constelacao visivel.
typedef struct {
    SatelliteInfo_t satellites[MAX_SATS_TRACKED];
    uint8_t count;
} SatelliteSnapshot_t;

// Data e hora UTC vindas do proprio GPS.
//
// Por que isto importa: o tempo do sistema e o uptime, que volta a zero a cada
// reinicio. Para nomear um arquivo por dia e para tracar orbita entre sessoes,
// so serve a hora do satelite - ela e a mesma para todo mundo, sempre.
typedef struct {
    bool     valida;    // ha data E hora conhecidas
    bool     fix;       // o receptor declarou posicao valida (campo A do RMC)
    uint16_t ano;       // 2026
    uint8_t  mes, dia;
    uint8_t  hora, min, seg;
} nmea_tempo_t;

// Parseia uma sentenca RMC. Devolve true se extraiu data E hora.
//
// $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
//         |     |                                    |
//         |     status: A=valido V=aviso             ddmmyy
//         hhmmss.ss UTC
bool nmea_parse_rmc(const char *sentence, nmea_tempo_t *saida);

// Posicao do RECEPTOR - a diferenca entre "onde os satelites estao" (GSV, ja
// existia) e "onde EU estou" (GGA, e o que faltava).
//
// O GGA e a sentenca canonica de posicao: e a unica que traz ALTITUDE junto
// com latitude/longitude, alem do HDOP (o quao boa e a geometria dos
// satelites usados) e da QUALIDADE do fix (GPS simples, DGPS, RTK...) - dado
// que nem o RMC nem o GSV entregam.
typedef struct {
    bool     valida;        // a sentenca chegou e tinha os campos esperados

    // GRAUS DECIMAIS, ja com o sinal do hemisferio aplicado: negativo e
    // Sul/Oeste, positivo e Norte/Leste. E o formato que qualquer coisa que
    // plota mapa (incluindo o proprio Google Maps na barra de busca) aceita
    // direto, sem o observador ter que fazer conta de grau/minuto na cabeca.
    double   lat;
    double   lon;

    float    altitude_m;    // acima do nivel do mar (referencia WGS84/geoide)
    float    hdop;          // diluicao de precisao horizontal - menor e melhor

    // 0=sem fix 1=GPS 2=DGPS 4=RTK fixo 5=RTK flutuante 6=estimado(dead reckoning)
    // SEM FIX (0), lat/lon vem ZERADOS de proposito - ver nmea_parse_gga().
    uint8_t  qualidade;

    uint8_t  sats_usados;   // quantos ENTRARAM NA CONTA do fix (!= "vistos" do GSV)
} nmea_posicao_t;

// $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
//               |lat    |  |lon     |  |  |  |  |altitude
//               |       |  |        |  |  |  |  M = metros
//               |       |  |        |  |  |  HDOP
//               |       |  |        |  |  sats usados no fix
//               |       |  |        |  qualidade (0=sem fix)
//               |       N/S|        E/W
// Devolve true quando a sentenca tinha o campo de qualidade (ela sempre traz,
// mesmo sem fix - e por isso este parser nao exige fix para devolver true).
bool nmea_parse_gga(const char *sentence, nmea_posicao_t *saida);

// Checksum NMEA: XOR de tudo entre '$' e '*'.
bool nmea_checksum_ok(const char *sentence);

// Extrai o N-esimo campo separado por virgula. Devolve string vazia se o campo
// nao existir - comum em NMEA, campos faltam com frequencia.
void nmea_campo(const char *sentence, int indice, char *out, size_t out_tam);

// Acumula uma sentenca GSV. As GSV chegam em varias mensagens por rodada, entao
// o parser guarda estado interno ate a rodada fechar.
// Devolve true quando o snapshot ficou COMPLETO - e so entao vale desenhar.
bool nmea_parse_gsv(const char *sentence, SatelliteSnapshot_t *saida);

// Zera o acumulador. Chamar ao abrir o app, para nao herdar estado de uma
// sessao anterior.
void nmea_reset(void);
