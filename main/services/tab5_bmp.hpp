// GRAVAR IMAGEM RGB565 COMO .BMP no cartao.
//
// 16 bits com as mascaras de cor no cabecalho (BI_BITFIELDS) - abre em
// qualquer visualizador. Monta o cartao sozinha (e desmonta), entao serve de
// qualquer tarefa: o worker do sistema (app Camera) ou o console.
//
// ESCRITA EM BLOCOS GRANDES. A versao anterior (dentro da HAL da camera)
// escrevia uma LINHA por fwrite (1440 bytes): o FAT reescrevia setores
// parciais a cada chamada, e 1,4 MB levavam mais de 10 s - com o preview
// parado e o app travado esse tempo todo. Aqui as linhas sao juntadas num
// bloco de 32 KB na PSRAM e gravadas de uma vez.
//
// Lento de proposito para a tarefa de INTERFACE: chame do worker
// (sistema_trabalho) ou de uma tarefa de fundo.

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Ajuste opcional aplicado a cada linha NA COPIA que vai para o arquivo (o
// buffer de origem nao e alterado). Roda junto da escrita, linha a linha - sem
// passada extra pela imagem inteira na PSRAM.
typedef void (*tab5_bmp_filtro_t)(uint16_t* linha, int largura);

// 'nome' sem extensao. 'dados' = largura x altura RGB565, linha a linha de cima
// para baixo (a ordem da tela).
bool tab5_bmp_salvar(const char* pasta, const char* nome, const uint16_t* dados, int largura, int altura,
                     tab5_bmp_filtro_t filtro = nullptr);
