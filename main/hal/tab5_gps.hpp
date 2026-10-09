// Servico de GPS - leitura de UART e parse de NMEA no Grove, independente de
// qual app esta na tela.
//
// MESMO MOLDE do tab5_mesh: o KERNEL liga e desliga este servico (Grove
// hot-swap - ver kernel/core.cpp), nao o app. Antes, a leitura de
// GPS vivia DENTRO do app Azimutal - UART instalada em ao_abrir(),
// desinstalada em ao_fechar(). Funcionava, mas so o Azimutal enxergava a
// posicao: nenhum outro app tinha como perguntar "onde estou" (o Mesh
// precisou disso para localizacao ao vivo, e foi o gatilho da mudanca).
//
// Agora quem le a UART e faz o parse de RMC/GGA (hora e posicao do
// RECEPTOR) e este servico. O Azimutal continua dono do que so ele precisa -
// o skyplot da constelacao (GSV) e o historico gravado no cartao - e para
// isso se inscreve para receber as sentencas cruas (ver tab5_gps_assinar).
//
// POSSE DO GROVE: exatamente a mesma arbitragem de sempre
// (tab5_grove_tomar/soltar, GROVE_DONO_GPS) - so que agora quem toma e o
// servico, no boot ou na troca de modulo, e nao mais o app ao abrir.

#pragma once

#include "nmea.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>
#include <stdint.h>

// Toma o Grove (GROVE_DONO_GPS) e sobe a UART + a tarefa de leitura. Chamar
// so com o Grove livre e um GPS de verdade detectado - normalmente e o
// kernel quem decide isso (ver tab5_grove_detectar() em kernel/core.cpp).
bool tab5_gps_iniciar(void);

// Desce a tarefa, fecha a UART e devolve o Grove. false = nao conseguiu
// encerrar a tempo (mesma semantica de tab5_mesh_parar) - nada foi
// desmontado nesse caso.
bool tab5_gps_parar(void);

bool tab5_gps_servico_ativo(void);

// Ultima posicao conhecida do RECEPTOR (nao dos satelites - isso e do
// Azimutal). 'valida' fica false ate a primeira GGA chegar; 'qualidade' 0
// quer dizer "o receptor fala, mas ainda sem fix".
nmea_posicao_t tab5_gps_posicao(void);

// Data/hora UTC do proprio GPS, e se ha FIX (campo A do RMC).
nmea_tempo_t tab5_gps_tempo(void);

// Ha sentenca recente? (mesmo espirito do tab5_mesh_no_ar()).
bool tab5_gps_no_ar(void);

// Ha quantos ms nao chega NENHUMA sentenca - usado pela vigia do Grove em
// kernel/core.cpp para saber quando o modulo sumiu do cabo, do mesmo jeito que
// tab5_mesh_silencio_ms() ja fazia para o radio.
uint32_t tab5_gps_silencio_ms(void);

// ASSINATURA DE SENTENCAS CRUAS - para quem precisa de algo alem de
// posicao/hora (hoje, so o Azimutal, para GSV/skyplot).
//
// UM ASSINANTE SO, de proposito: e o mesmo espirito de "quem toma deve
// soltar" do Grove - simplicidade sobre generalidade que ninguem pediu
// ainda. Se um dia mais de um precisar, o jeito e virar uma lista pequena,
// nao complicar isto agora sem uso real.
//
// A queue recebe TODA sentenca valida (checksum ok), inclusive as que o
// proprio servico ja processou (RMC/GGA) - filtrar pelo prefixo e trabalho
// de quem assina, como o Azimutal ja fazia antes desta mudanca.
bool tab5_gps_assinar(QueueHandle_t fila);
void tab5_gps_dessinscrever(void);
