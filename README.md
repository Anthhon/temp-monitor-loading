# temp-monitor-loading

Esse repositório contém a minha solução para o desafio da 3º etapa do processo seletivo organizado pela Loading.

O desafio se trata da criação de um sistema de monitoramento de temperatura, embarcado em um ESP32-S3, com algumas especificações técnicas sobre a implementação do mesmo.

# Requisitos do sistema

- [X] Leituras periódicas do sensor de temperatura do aparelho
- [X] Exibir os valores de temperatura lidos de maneira clara
    - [X] Monitor serial
- [X] Estados relativos a temperatura do aparelho
    - [X] Falha/Indefinido
    - [X] Normal
    - [X] Alerta
    - [X] Crítico
    - [X] Exibir alterações de estados pelo led RGB 
- [X] Leitura inválida de temperatura
    - [X] Simular leitura inválida
    - [X] Alertar condição de falha
- [X] Definição de valores limites de temperatura
    - [X] Alterar em run-time (Opcional)
