# **Relatório: Código do Enzo Shinzato**

**Nome dos integrantes:**
- Alexander Oliveira.
- Enzo Shinzato.
- Henrique Lima.

## **Avaliação inicial do cenário inicial**
Pela descrição do funcionamento do código, é possível entender que a intenção do código é criar duas threads concorrentes de mesma prioridade que incrementam uma mesma variável. Ao final dos incrementos o código compara o valor contado para a variável de incremento e compara com o valor esperado. Se forem iguais, não houve erros de race condition e a placa acende o LED verde. Se os valores forem diferentes, o LED vermelho acende, sinalizando uma falha na contagem da variável ocasionada por uma race condition.


## **Casos de teste**
| Caso de Teste | Pré-condição | Etapas de Teste | Pós-condição Esperada |
|----------------|---------------|------------------|------------------------|
| 1. Execução concorrente com troca de contexto forçada | 2 Threads concorrentes de prioridade 7 que incrementam uma variável global em um valor de 100000 e possuem uma função k_yield() antes do registro do incremento. | 1. As duas threads começam a incrementar shared_counter simultaneamente.<br>2. A execução termina e o valor final é exibido no console. | O valor final de shared_counter é menor que 200000, indicando perda de incrementos devido à condição de corrida. O LED vermelho é aceso. |
| 2. Execução concorrente de threads com atraso intencional | 2 Threads concorrentes de prioridade 7, que incrementam uma variável global em um valor de 100000 e possuem uma função k_busy_wait() antes do registro do incremento. | 1. As duas threads começam a incrementar shared_counter simultaneamente.<br>2. A execução termina e o valor final é exibido no console. | O valor final de shared_counter é menor que 200000, indicando perda de incrementos devido à condição de corrida. O LED vermelho é aceso. |
| 3. Execução Concorrente com troca de contexto forçada (carga menor) | 2 Threads concorrentes de prioridade 7 que incrementam uma variável global em um valor de 1000 e possuem uma função k_yield() antes do registro do incremento. | 1. As duas threads começam a incrementar shared_counter simultaneamente.<br>2. A execução termina e o valor final é exibido no console. | O valor final de shared_counter é menor que 2000, indicando perda de incrementos devido à condição de corrida. O LED vermelho é aceso. |


## **Descrição da race condition**
O código força uma race condition ao criar duas threads de mesma prioridade competindo pelo acesso a uma mesma variável global. Ao utilizar o comando k_yield() antes do registro do incremento da variavel global pela thread que estava sendo executada, a outra thread passa a executar e incrementar baseado no valor inicial (não incrementado) da variável global. O resultado é uma inconsistência do resultado contado da variável pelas duas threads e pelo resultado esperado (o que é sinalizado pelo LED vermelho).
Ao executar o código no primeiro caso de teste, obtém-se o seguinte output no terminal:

![Imagem 1: Output do código não corrigido no caso de teste 1](C:\Users\alexa\Pictures\Screenshots/Captura de tela 2025-10-30 011053.png)

Na execução do código no segundo caso de teste, obtém-se o seguinte output:

![Imagem 2: Output do código não corrigido no caso de teste 2](C:\Users\alexa\Pictures\Screenshots/Captura de tela 2025-10-30 015910.png)

Na execução do código no terceiro caso de teste, obtém-se o seguinte output:

![Imagem 2: Output do código não corrigido no caso de teste 2](C:\Users\alexa\Pictures\Screenshots/Captura de tela 2025-10-30 020412.png)

## **Descrição da solução**


## **Avaliação do colega**


