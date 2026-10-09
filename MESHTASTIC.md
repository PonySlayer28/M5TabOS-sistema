# Integração com o Meshtastic

Mapa do que o protocolo do Meshtastic oferece a um cliente (o papel do Tab5
diante do nó) e do que o M5tabOS usa ou pode usar. Conferido contra os
`.pb.h` gerados do firmware (`src/mesh/generated/meshtastic/`) e o
`AdminModule.cpp`.

## O teto de nós do rádio

O número de nós que um rádio guarda é **constante de compilação do firmware**
(`MAX_NUM_NODES`, `src/mesh/mesh-pb-constants.h`), não configuração:

| Firmware | ESP32-S3 (Heltec V3, T-Beam S3) | ESP32 genérico, C3, **C6 (Unit C6L)**, nRF52840 |
|---|---|---|
| até 2.7.x | 100 / 200 / 250 conforme a flash (4 / 8 / 16 MB) | **100** |
| 2.8.0+ | igual | **120** + "warm tier" (identidade e chave dos despejados) |

Um Heltec V3 com 8 MB de flash guarda 200, e o Unit C6L guarda 100: os mesmos
nós ouvidos e listas de tamanho diferente. O "nós online/total" do `LocalStats`
é a visão do rádio e para no teto dele. Atualizar o C6L para a 2.8 sobe o teto
para 120; mais do que isso, só fora do rádio.

**Fora do rádio é onde o Tab5 ganha.** A tabela do serviço (256 vagas, PSRAM)
aprende também pelos pacotes ao vivo (NODEINFO, POSITION, TELEMETRY, texto), não
só pelo dump. Com uma NodeDB própria no cartão (todos os campos, não só o
nome), o Tab5 passa a lembrar mais nós do que o rádio consegue guardar.
`MyNodeInfo.nodedb_count` (campo 15) diz quantos o rádio tem, para mostrar os
dois números lado a lado.

## Como o cliente fala com o nó

- **FromRadio** (do nó para nós): `1 id, 2 packet, 3 my_info, 4 node_info,
  5 config, 6 log_record, 7 config_complete_id, 8 rebooted, 9 moduleConfig,
  10 channel, 11 queueStatus, 13 metadata, 14 mqttClientProxyMessage,
  15 fileInfo, 16 clientNotification, 17 deviceuiConfig, 19 region_presets`.
- **ToRadio** (de nós para o nó): `1 packet, 3 want_config_id, 4 disconnect,
  7 heartbeat`.
- **Administração** é um `MeshPacket` comum: destino = o próprio nó, porta
  `ADMIN_APP` (6), payload = `AdminMessage`. Sem `from` (= 0), o firmware o
  trata como admin LOCAL de um cliente conectado e **não pede chave de
  sessão** (`AdminModule.cpp`, ramo `mp.from == 0`). A chave (`session_passkey`)
  só é exigida para administrar um nó REMOTO.

## AdminMessage - o que dá para fazer

| Campo | Operação | Observação |
|---|---|---|
| 1/2 | `get_channel_request` / resposta | um canal por pedido |
| 3/4 | `get_owner_request` / resposta (User) | |
| 5/6 | `get_config_request` (tipo) / resposta | o dump já traz todas |
| 7/8 | `get_module_config_request` / resposta | idem |
| 12/13 | `get_device_metadata` | firmware, capacidades, hw |
| 32 | `set_owner` (User) | nome longo/curto, `is_licensed`, `is_unmessagable` |
| 33 | `set_channel` (Channel) | nome, PSK, papel, uplink/downlink MQTT, precisão de posição |
| 34 | `set_config` (Config) | **substitui a SEÇÃO inteira** (ver abaixo) |
| 35 | `set_module_config` | idem, por módulo |
| 38 | `remove_by_nodenum` | tira um nó da NodeDB do rádio |
| 39/40 | `set/remove_favorite_node` | **em uso** (estrela) |
| 41/42 | `set/remove_fixed_position` | posição fixa - pode vir do GPS do Tab5 |
| 43 | `set_time_only` | acerta o relógio do nó |
| 47/48 | `set/remove_ignored_node` | bloquear um nó |
| 49 | `toggle_muted_node` | silenciar um nó |
| 64/65 | `begin/commit_edit_settings` | transação: várias mudanças, uma gravação |
| 97 | `reboot_seconds` | reiniciar |
| 98 | `shutdown_seconds` | desligar |
| 99 / 94 | `factory_reset_config` / `factory_reset_device` | |
| 100 | `nodedb_reset` | esvaziar a NodeDB do rádio |

### Mudar configuração sem modelar o protobuf inteiro

`set_config` grava a seção inteira (`config.lora = c.payload_variant.lora`):
mandar só o campo alterado zera os outros. A saída que mantém o parser
pequeno do serviço:

1. Guardar os **bytes crus** de cada seção recebida no dump (FromRadio 5 e 9).
2. Para alterar, mandar os bytes guardados **mais** o campo novo no fim. Em
   protobuf, campo escalar repetido vale a última ocorrência; o nanopb do
   firmware decodifica assim. Não vale para campo `repeated` (ex.:
   `ignore_incoming`), que acumularia.
3. Envolver em `begin_edit_settings` … `commit_edit_settings`. O firmware
   grava e, se a seção exigir, **reinicia sozinho**; o `rebooted` (FromRadio 8)
   avisa, e o serviço já pede a configuração de novo.

### Seções e os campos que interessam

- **device** (Config 1): `role` (CLIENT, CLIENT_MUTE, ROUTER, REPEATER,
  TRACKER, SENSOR, CLIENT_HIDDEN, ROUTER_LATE, CLIENT_BASE…),
  `rebroadcast_mode`, `node_info_broadcast_secs`, `tzdef`,
  `led_heartbeat_disabled`.
- **position** (2): `position_broadcast_secs`, `smart_enabled`,
  `fixed_position`, `gps_mode` (**deve ficar NOT_PRESENT no C6L ligado ao
  Grove**: o GPS toma os mesmos pinos da serial), `broadcast_smart_minimum_*`.
- **power** (3): `is_power_saving`, `ls_secs`, `min_wake_secs`.
- **network** (4): `wifi_enabled`, `wifi_ssid`, `wifi_psk`, `ntp_server` -
  o Wi-Fi do próprio C6L, que alimenta o MQTT dele.
- **display** (5): tela do C6L (`screen_on_secs`, `flip_screen`, `units`).
- **lora** (6): `region` (BR_902 = 26), `modem_preset` (LONG_FAST = 0 …),
  `hop_limit`, `tx_enabled`, `tx_power`, `channel_num`,
  `sx126x_rx_boosted_gain`, `ignore_mqtt`, `config_ok_to_mqtt`.
- **bluetooth** (7): `enabled`, `mode` (PIN aleatório/fixo/sem), `fixed_pin`.
- **security** (8): chaves, `admin_key` (até 3), `serial_enabled`,
  `admin_channel_enabled`. **Não mexer** sem saber: trava o nó para fora.

ModuleConfig: `1 mqtt, 2 serial, 3 external_notification, 4 store_forward,
5 range_test, 6 telemetry, 7 canned_message, 9 remote_hardware,
10 neighbor_info, 11 ambient_lighting, 12 detection_sensor, 13 paxcounter,
14 statusmessage, 15 traffic_management, 17 mesh_beacon`. Os úteis aqui:
**mqtt** (ligar, servidor, uplink/downlink por canal, mapa), **telemetry**
(intervalos), **neighbor_info**, **store_forward**, **range_test**.

**Serial (ModuleConfig 2) é a corda que nos prende ao nó**: mudar `mode`,
pinos ou baud derruba o enlace do Grove. Fica de fora da edição.

## Portas de aplicação

Em uso: `1 TEXT, 3 POSITION, 4 NODEINFO, 5 ROUTING, 6 ADMIN, 67 TELEMETRY`.
Úteis para integrar: `8 WAYPOINT` (pontos no mapa), `70 TRACEROUTE` (a rota
até um nó, salto a salto, com SNR de cada um), `71 NEIGHBORINFO` (vizinhos
diretos de cada nó - o grafo da malha), `65 STORE_FORWARD` (histórico
guardado por um roteador), `66 RANGE_TEST`, `73 MAP_REPORT`,
`12 KEY_VERIFICATION`.

## Outras mensagens que o nó manda e hoje se perdem

- `metadata` (13): já lido só o firmware; traz também `hasWifi`,
  `hasBluetooth`, `role`, `hw_model`, `hasPKC`, `excluded_modules`.
- `my_info` (3): só o número; traz `reboot_count`, `nodedb_count`,
  `pio_env`, `firmware_edition`.
- `clientNotification` (16): avisos do firmware para o usuário ("favoritos
  cheios", chave duplicada, chave fraca). Deveriam virar notificação na barra.
- `log_record` (6): o log do nó, para diagnóstico.
- `NodeInfo` 10–13: `is_favorite` (em uso), `is_ignored`,
  `is_key_manually_verified`, `is_muted`.

## O que já está implementado

- Fases 1, 2 e 5 abaixo: `services/tab5_mesh_admin.{hpp,cpp}` e a página "Nó"
  do app Mesh; NodeDB do Tab5 em `nodedb.bin`.
- Conferido nas versões 2.5, 2.7 e atual do firmware: números de campo,
  dispensa de chave no admin local e reinício no `commit_edit_settings`.
- `ferramentas/teste_mesh_admin.py` prova a montagem com os protobufs oficiais.

## Roteiro proposto

1. **Ver o nó** (só leitura, sem risco): guardar Config/ModuleConfig/
   metadata/my_info do dump; página "Nó" com firmware, hardware, região,
   preset, papel, saltos, potência, MQTT, Bluetooth, posição; nós do rádio
   (`nodedb_count`) ao lado dos do Tab5; `clientNotification` vira aviso.
2. **Ajustes seguros**, um toque e o nó grava e reinicia: nome longo/curto,
   papel, `hop_limit`, `tx_power`, intervalos de posição/NodeInfo/telemetria,
   MQTT (ligar, uplink/downlink), Bluetooth (ligar, PIN fixo), posição fixa
   vinda do GPS do Tab5, acertar a hora do nó.
3. **Ajustes de alto impacto**, com confirmação: região, preset do modem,
   canais (criar, renomear, PSK, papel, apagar), reiniciar, desligar,
   esvaziar a NodeDB, tirar/ignorar/silenciar um nó.
4. **Malha**: traceroute a partir do cartão do nó, vizinhos (NeighborInfo),
   waypoints, teste de alcance.
5. **NodeDB própria no cartão**: todos os campos de todo nó visto, além do
   teto do rádio.
