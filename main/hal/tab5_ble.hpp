// BLUETOOTH DO SISTEMA - BLE pelo C6 interno do Tab5.
//
// O P4 nao tem radio. O controlador BLE mora no C6 (o mesmo chip do WiFi e do
// ESP-NOW), e o firmware nosso do C6 ja nasce com ele ligado; os pacotes HCI
// passam pelo mesmo SDIO do WiFi (esp_hosted). Aqui mora a STACK - NimBLE, no
// P4 - e o que todo mundo que usa Bluetooth precisa em comum:
//
//   - ligar e desligar (o C6 reveza o radio entre WiFi, ESP-NOW e BLE sozinho;
//     os tres convivem, cada um um pouco mais lento);
//   - VARRER: quem esta por perto, com nome, sinal e se e um no Meshtastic.
//
// Conexao e GATT ficam com quem conversa (ver services/tab5_mesh_ble.cpp): a
// NimBLE entrega os eventos de uma conexao ao callback de quem a abriu, e cada
// protocolo tem o seu jeito de ler e escrever.
//
// So BLE: o C6 nao faz Bluetooth classico (fone, caixa de som antiga).
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  TAB5_BLE_DESLIGADO = 0,
  TAB5_BLE_SUBINDO,      // esperando o C6 e a stack sincronizarem
  TAB5_BLE_PRONTO,
  TAB5_BLE_FALHOU,       // o C6 nao deu o controlador (firmware sem BT?)
  TAB5_BLE_DESLIGANDO,   // pedido de desligar aceito; a pilha esta descendo
} tab5_ble_estado_t;

// Cria a tarefa. Nao liga nada - como o tab5_wifi_iniciar().
void tab5_ble_iniciar(void);

// Liga/desliga a stack. Ligar sobe o enlace com o C6 se preciso (trilho do
// radio = I2C): chamar da tarefa de interface. Assincrono.
void tab5_ble_ligar(void);
void tab5_ble_desligar(void);

tab5_ble_estado_t tab5_ble_estado(void);
bool        tab5_ble_pronto(void);
const char* tab5_ble_descricao(void);

// ---------------------------------------------------------------------------
// VARREDURA - quem esta por perto
// ---------------------------------------------------------------------------
#define TAB5_BLE_VISTOS 24

// O QUE E O APARELHO, pelo que ele anuncia (sem conectar). Rastreador e
// etiqueta de achar coisas - AirTag e a rede Find My da Apple, Tile, SmartTag
// da Samsung, a rede Encontre Meu Dispositivo do Google; beacon e o farol
// fixo de loja/museu (iBeacon, Eddystone).
typedef enum {
  TAB5_BLE_OUTRO = 0,
  TAB5_BLE_MESHTASTIC,
  TAB5_BLE_RASTREADOR,
  TAB5_BLE_BEACON,
} tab5_ble_classe_t;

typedef struct {
  uint8_t  addr[6];      // ordem da NimBLE (byte 0 = o ultimo do texto)
  uint8_t  tipo;         // BLE_ADDR_PUBLIC / RANDOM
  int8_t   rssi;         // media movel: um relato isolado varia 5-10 dB
  bool     meshtastic;   // anuncia o servico do Meshtastic
  uint8_t  classe;       // tab5_ble_classe_t
  char     marca[16];    // "Find My", "Tile", "iBeacon"... ("" = nao sabe)
  int8_t   p1m;          // RSSI a 1 m que o proprio anuncio declara; 0 = nao declarou
  char     nome[24];
  uint32_t visto_ms;
} tab5_ble_visto_t;

// OUVINTE DE ANUNCIOS - o relato CRU, um por pacote recebido, sem a media
// movel da lista acima. Para quem precisa do instante exato de cada leitura
// (o radar cruza o RSSI com o rumo do aparelho no momento em que o pacote
// chegou). Roda na tarefa da NimBLE: nao bloqueie, nao desenhe, nao fale I2C -
// so enfileire e volte. Um ouvinte so; nullptr remove.
typedef void (*tab5_ble_ouvinte_t)(const uint8_t addr[6], int8_t rssi);
void tab5_ble_ouvinte(tab5_ble_ouvinte_t f);

// Cancela a varredura em curso (no-op se nao ha).
void tab5_ble_varredura_parar(void);

// Varre por 'ms' (sem esperar). false se a stack nao esta pronta ou ja ha uma
// varredura/conexao usando o radio. A lista NAO e zerada: quem sumiu envelhece.
bool     tab5_ble_varrer(uint32_t ms);
bool     tab5_ble_varrendo(void);
int      tab5_ble_n_vistos(void);
bool     tab5_ble_visto(int i, tab5_ble_visto_t* out);
// Muda a cada novidade (estado, varredura, aparelho novo): a tela compara
// com o que desenhou e so repinta quando muda.
uint32_t tab5_ble_versao(void);

// "AA:BB:CC:DD:EE:FF" <-> ordem da NimBLE.
void tab5_ble_endereco_txt(const uint8_t a[6], char out[18]);
bool tab5_ble_endereco_ler(const char* txt, uint8_t a[6]);

// O UUID do servico do Meshtastic (quem conecta nele precisa do mesmo).
#define TAB5_BLE_UUID_MESHTASTIC "6ba1b218-15a8-461f-9fa8-5dcae273eafd"
