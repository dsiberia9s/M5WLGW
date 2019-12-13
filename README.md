# ![📟](https://vk.com/emoji/e/f09f939f.png) M5Stack Wi-Fi <— UDP —> LoRa Gateway

**[по-русски]**

Шлюз вещает сигнал-маяк, в котором содержится диапазон доступных портов. Клиент на основании сигнала-маяка выбирает желаемый порт и сообщает шлюзу о намерении его занять. Если порт свободен, то шлюз соглашается или отвергает запрос. Если порт присвоен клиенту, то он может принимать или отправлять UDP-пакеты через шлюз. IP-адрес шлюза является общим для всех клиентов, а порт у каждого свой. Порядок работы отображён в таблице 1.

Таблица 1

| [  клиент  ] |    <— сигнал-маяк —     | [  шлюз  ] | **0xA1** |
| :----------: | :---------------------: | :--------: | :------: |
| [  клиент  ] |   — выбранный порт —>   | [  шлюз  ] | **0xB1** |
| [  клиент  ] | <— согласие или отказ — | [  шлюз  ] | **0xA2** |
| [  клиент  ] |     — UPD-пакет —>      | [  шлюз  ] | **0xB2** |
| [  клиент  ] |     <— UPD-пакет —      | [  шлюз  ] | **0xA3** |

**[English]**

The gateway broadcasts a beacon, which contains a range of available ports. Based on the beacon, the client selects the desired port and informs the gateway about its intention to occupy it. If the port is free, then the gateway agrees or rejects the request. If the port is assigned to the client, then it can receive or send UDP packets through the gateway. The IP address of the gateway is common to all clients, and each port has its own. The operating procedure is shown in table 1.

Table 1

| [  client  ] |    <— beacon —     | [  gateway  ] | **0xA1** |
| :----------: | :----------------: | :-----------: | :------: |
| [  client  ] | — selected port —> | [  gateway  ] | **0xB1** |
| [  client  ] | <— OK or decline — | [  gateway  ] | **0xA2** |
| [  client  ] |  — UPD-packet —>   | [  gateway  ] | **0xB2** |
| [  client  ] |  <— UPD-packet —   | [  gateway  ] | **0xA3** |



