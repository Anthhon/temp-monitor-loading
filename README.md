# temp-monitor-loading

Esse repositório contém a minha solução para o desafio da 3º etapa do processo seletivo organizado pela Loading.

O desafio se trata da criação de um sistema de monitoramento de temperatura, embarcado em um ESP32-S3, com algumas especificações técnicas sobre a implementação do mesmo.

# Requisitos do sistema

- [X] Leituras periódicas do sensor de temperatura do aparelho
- [ ] Exibir os valores de temperatura lidos de maneira clara
    - [X] Monitor serial
- [ ] Definição de valores limites de temperatura
    - [ ] Alterar em run-time (Opcional)
- [ ] Estados relativos a temperatura do aparelho
    - [ ] Falha/Indefinido
    - [ ] Normal
    - [ ] Alerta
    - [ ] Crítico
    - [ ] Exibir alterações de estados pelo led RGB 
- [ ] Leitura inválida de temperatura
    - [ ] Simular leitura inválida
    - [ ] Alertar condição de falha
