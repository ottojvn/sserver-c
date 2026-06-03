# Servidor HTTP do Zero (Raw TCP Sockets) — Socratic Roadmap

**Core Principle:** "Never eliminate the student's struggle — elevate it."

## Visão Geral
Construir um servidor HTTP em C usando apenas sockets TCP POSIX (sem bibliotecas prontas de web). 
Este documento define *o que* deve ser alcançado em cada fase e *quais os problemas e regras* que você enfrentará, mas **não te diz como fazer**. Cabe a você pesquisar as APIs do sistema operacional, analisar as especificações e quebrar a cabeça para encontrar os mecanismos adequados.

---

## FASE 0 — Fundação
> **Status:** OK
Configuração do build system e esqueleto básico do projeto em C.

---

## FASE 1 — O Ponto de Entrada (Socket)
> **Status:** OK
**Objetivo:** Obter um descritor de arquivo do SO que represente uma extremidade de comunicação de rede orientada a fluxo (TCP/IPv4).

---

## FASE 2 — O Endereço e o Tradutor
> **Status:** OK
**Objetivo:** Configurar as informações de roteamento (Família de IP, Porta e Endereço Local) e lidar com as idiossincrasias do formato de representação de dados de redes (Network Byte Order vs Host Byte Order).

---

## FASE 3 — A Reserva de Domínio (Bind)
> **Status:** OK
**Objetivo:** O Sistema Operacional precisa ser avisado de que o seu socket exige exclusividade sobre o endereço que você configurou. Amarre o seu socket à porta escolhida.
**Critérios de Sucesso e Desafios:**
- A chamada de sistema apropriada não deve retornar erro. Você precisará lidar com um problema comum de compatibilidade de ponteiros do C antigo.
- **Desafio de Resiliência da Porta:** Se você encerrar seu servidor abruptamente (Ctrl+C) e tentar iniciá-lo de novo na mesma hora, ele provavelmente travará reclamando que a porta já está em uso (o famoso estado TIME_WAIT). Descubra como modificar o comportamento padrão do seu socket *antes* da reserva para que ele permita a reutilização imediata do endereço.

---

## FASE 4 — A Recepção
> **Status:** OK
**Objetivo:** Transmutar o canal de comunicação para um estado de espera passiva e, em seguida, efetivamente atender a primeira conexão externa que bater à sua porta.
**Critérios de Sucesso e Desafios:**
- Como você faz para que o servidor não encerre a execução imediatamente, mas sim aguarde passivamente por um visitante?
- Ao estabelecer o contato com o cliente (seu navegador, por exemplo), como você descobre de onde ele está vindo (qual é o seu endereço de origem)? Seu objetivo é imprimir essa informação no terminal.
- *Paradigma a investigar:* A operação que efetiva o contato resulta na criação de um *novo* canal, separado do canal original que estava aguardando. Por que o sistema operacional adota essa arquitetura de dois canais distintos?

---

## FASE 5 — Espionando o Protocolo
> **Status:** Em andamento.
**Objetivo:** Imediatamente após a conexão ser concretizada, o visitante enviará uma mensagem inicial. Seu dever é interceptá-la e inspecioná-la.
**Critérios de Sucesso e Desafios:**
- Como você captura os dados brutos que chegam através do novo canal de conversação?
- Você precisará de um espaço na memória para armazenar esses dados assim que chegarem. Imprima tudo o que capturar no terminal.
- Ao observar a mensagem interceptada: Qual é a estrutura dessa mensagem? Como o cliente sinaliza o fim de uma linha e o fim de toda a mensagem?

---

## FASE 6 — Falando o Idioma
**Objetivo:** O cliente espera uma resposta em uma gramática rigorosa. Seu objetivo é estruturar uma mensagem que seja compreendida como válida pelo navegador.
**Critérios de Sucesso e Desafios:**
- O que acontece se você enviar um texto qualquer de volta? Como a especificação do protocolo define que uma resposta bem-sucedida deve ser estruturada?
- Qual é o conjunto mínimo absoluto de informações necessárias para que o navegador exiba uma página HTML simples?
- Seu objetivo final é ver a mensagem de sucesso renderizada no navegador.
- Como você sinaliza ao cliente e ao sistema operacional que a conversa acabou e os recursos podem ser liberados?

---

## FASE 7 — O Roteador Básico
**Objetivo:** Atualmente, o servidor responde de forma estática. Ele precisa ser capaz de interpretar a requisição e tomar decisões baseadas no que o cliente está pedindo.
**Critérios de Sucesso e Desafios:**
- Analisando a mensagem interceptada na Fase 5, como você pode isolar programaticamente apenas a informação de qual "recurso" o cliente deseja acessar?
- Crie comportamentos distintos baseados no recurso solicitado (ex: página inicial vs rota de verificação de status).
- O que deve acontecer quando o cliente solicita algo que não existe? Qual é a forma normatizada pelo protocolo de comunicar essa falha?

---

## FASE 8 — Concorrência e Escalonamento
**Objetivo:** Simule uma requisição que demore vários segundos para ser processada. O que acontece com um segundo cliente que tenta se conectar durante esse tempo? Precisamos resolver esse gargalo.
**Critérios de Sucesso e Desafios:**
- Como o sistema operacional permite que seu programa execute múltiplas tarefas ao mesmo tempo ou gerencie múltiplas conexões de forma não-bloqueante?
- Pesquise e compare as diferentes arquiteturas de servidores concorrentes (quais são os modelos baseados em isolamento, memória compartilhada ou orientação a eventos?).
- Escolha uma estratégia, justifique a sua escolha e implemente a capacidade do servidor lidar com conexões simultâneas.

---

## FASE 9 — Separando Código e Dados
**Objetivo:** O conteúdo não deve viver dentro do seu código fonte. O servidor deve ser capaz de servir arquivos reais que habitam o disco rígido.
**Critérios de Sucesso e Desafios:**
- Dado que um cliente pede por um arquivo específico, como seu programa localiza, acessa e transfere o conteúdo desse arquivo de forma eficiente para a conexão de rede?
- Como o navegador descobre se os bytes que ele acabou de receber representam um documento de texto, uma imagem ou um vídeo? O que seu servidor precisa mudar na resposta ao servir arquivos de naturezas diferentes?
