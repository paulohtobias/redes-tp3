# Terceiro Trabalho Prático de Redes

## Millas N. R. Avellar, Paulo H. Tobias, Welton A. R. Santos

- **Compilar:**
	- Para compilar: `make`

- **Executar:**
	- Para executar o servidor: `./main.out`
	
	O executável gerado poderá rodar em modo de remetente e em modo de receptor. Para rodar em modo remetente, a flag `-f NOME` ou `-s MSG` deve ser usada. No caso de `-f`, `NOME` deve corresponder a um nome de um arquivo, que será enviado ao remetente. Para `-s`, `MSG` será enviada diretamente ao remetente. Se nenhuma das duas flags forem detectadas, então o programa rodará em modo remetente.

	A flag `-t TAM`, que é obrigatória, indica, para o remetente, o tamanho máximo da mensagem a ser enviada. No caso do receptor, `TAM` indica o tamanho do buffer para receber a mensagem corrigida, ou seja, `TAM` precisa ser maior ou igual ao tamanho da mensagem recebida.

	Para definir a taxa de inconsistência das transferências, as flags `-C PROB`, `-D PROB` e `-A PROB` devem ser usadas. `PROB` deve ser um valor inteiro entre 0 e 99 e indica a probabilidade de que a inconsistência definida ocorra durante o envio de um segmento. `-C` indica probabilidade de corrompimento, `D` é a probabilidade de descarte e `-A` a probabilidade de atrasos.

	As demais opções podem ser conferidas execurando `./main.out -h`:
	```
	Uso: ./main.out
	-q			Suprime todas as mensagens da saída padrão.
	-R	RTT		Estimated RTT em milissegundos. Padrão: 100
	-C	PROB		Probabilidade de corromper pacotes. Padrão: 0
	-D	PROB		Probabilidade de descartar pacotes. Padrão: 0
	-A	PROB		Probabilidade de atrasar pacotes. Padrão: 0
	-m	MAX		Número máximo de conexões simultâneas. Padrão: 3
	-B	BACKLOG		Quantidade de conexões enfileiradas. Padrão: 5
	-s	MSG		Mensagem a ser enviada. Padrão: ''
	-f	NOME		Nome do arquivo a ser enviado. Padrão: ''
	-i	IP		Endereço ip para enviar/receber mensagens. Padrão: '0.0.0.0'
	-p	PORTA		Porta usada pelo processo. Padrão: 9999
	-t	TAM		Tamanho da mensagem a ser enviada/recebida.
	```
