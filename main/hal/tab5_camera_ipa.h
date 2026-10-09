#pragma once

// ALGORITMO PROPRIO NO PIPELINE DO ISP (IPA) - ver o cabecalho de
// tab5_camera_ipa.c para o porque. Este header e o que o resto do sistema
// enxerga: uma foto do estado do ISP, para diagnostico (comando 'camera' do
// console).

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t quadros;          // quadros processados desde que a camera ligou
  float    ganho_r, ganho_b; // balanco de branco pedido pelo AWB
  float    ccm_awb[3][3];    // matriz de cor da calibracao (sem o balanco)
  float    ccm_isp[3][3];    // a que vai ao ISP, depois da protecao do branco
  float    g_branco_antes;   // verde de um branco saturado com a matriz da calibracao (1 = branco)
  float    g_branco_depois;  // o mesmo, com a matriz protegida
  uint32_t saturacao;        // 128 = 1,0
  uint32_t exposicao_us;
  float    ganho_sensor;
  uint32_t hist[16];         // histograma do Y (saida do ISP), faixas de 16
} tab5_cam_ipa_estado_t;

// Copia o ultimo estado. Falso se o pipeline ainda nao rodou nenhum quadro.
bool tab5_camera_ipa_estado(tab5_cam_ipa_estado_t* e);

#ifdef __cplusplus
}
#endif
