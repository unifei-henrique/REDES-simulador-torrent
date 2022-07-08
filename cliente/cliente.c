#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// #include "compartilhado.h"

#define PORT 1337

int checksum(char pacote[], int size) {
  int soma = 0;

  for (int i = 0; i < size; i++) {
    soma += pacote[i];
    soma %= 128;
  }

  return soma;
}

long encontrar_maior(long *vetor, long tamanho, long numero_sequencia) {
  long maior = -1;
  for (long i = 0; i < tamanho; i++) {
    if (vetor[i] > maior && vetor[i] < numero_sequencia) {
      maior = vetor[i];
    }
  }
  return maior;
}

typedef enum { false, true } bool;

int main() {
  struct sockaddr_in servidor;
  ssize_t ler_bytes, escrever_bytes;
  int porta, conector, socket_cliente, binder;
  long pacote_atual = 0, numero_pacotes = 0;
  int tamanho_buffer = 4096;
  unsigned long numero_sequencia;
  long *pacotes_recebidos;
  long vetor_zerado[4096];
  char nome_arquivo[200], resposta[30];

  memset(vetor_zerado, 0, 4096);

  socket_cliente = socket(AF_INET, SOCK_DGRAM,
                          0);  // Cria uma conexão socket no socket_cliente

  if (socket_cliente <= 0) {
    printf("Erro no socket: %s\n", strerror(errno));
    exit(1);
  }

  bzero(&servidor, sizeof(servidor));  // da memset no valor 0 em todo serv_addr

  servidor.sin_family =
      AF_INET;  // Termo que guarda o tipo de conexão (ipv4,ipv6...)
  servidor.sin_port = htons(PORT);  // Guarda a porta a se checar

  // Recebe o socket, a struct com as informações do server e o tamanho da
  // struct com as informações do server Essa função conecta cliente ao servidor
  // e retorna negativo se a conexão falhou
  binder = bind(socket_cliente, (const struct sockaddr *)&servidor,
                sizeof(servidor));
  if (conector < 0) {
    fprintf(stderr, "%s", "Falha na conecao\n");
    exit(1);
  } else {
    printf("[SERVIDOR CONECTADO]\n\n");
  }

  //***************************************************************
  //			           REQUISIÇÃO DE ARQUIVO
  //***************************************************************

  printf("Qual arquivo pedir: ");
  scanf("%s", nome_arquivo);

  // fgets(nome_arquivo, sizeof(nome_arquivo), stdin); //lê mensagem do terminal
  escrever_bytes =
      sendto(socket_cliente, nome_arquivo, sizeof(nome_arquivo), 0,
             (const struct sockaddr *)&servidor,
             sizeof(servidor));  // envia pelo socket_cliente o nome_arquivo

  if (escrever_bytes == 0) {  // se vc n enviar nada
    printf("Erro no write: %s\n", strerror(errno));
    printf("Nada escrito.\n");
    exit(1);
  }
  int servlen = sizeof(servidor);
  escrever_bytes = recvfrom(socket_cliente, &resposta, sizeof(nome_arquivo), 0,
                            (struct sockaddr *)&servidor, &servlen);

  if (escrever_bytes <= 0) {  // se vc n enviar nada
    printf("%s\n", strerror(errno));
    exit(1);
  }
  //***************************************************************
  //			         FIM REQUISIÇÃO DE ARQUIVO
  //***************************************************************
  FILE *file;
  char pacote[4106];
  int ack[2];
  ssize_t ler;

  //***************************************************************
  //			           RECEBIMENTO DO ARQUIVO
  //***************************************************************

  file = fopen(nome_arquivo, "wb");

  ler = recvfrom(socket_cliente, &numero_pacotes, sizeof(long), 0,
                 (struct sockaddr *)&servidor, &servlen);

  pacotes_recebidos = malloc(numero_pacotes * sizeof(long));

  while (pacote_atual != numero_pacotes) {
    int contador = 0;

    ler = recvfrom(socket_cliente, pacote, sizeof(pacote), 0,
                   (struct sockaddr *)&servidor, &servlen);

    while (pacote[4096] != checksum(pacote, tamanho_buffer)) {
      if (contador == 3) {
        printf("Falha ao transferir arquivo checksum não bateu\n");
        return 0;
      }
      contador++;
    }

    // Converte ultimos 8 bytes em int
    numero_sequencia =
        (pacote[4098] << 56) | ((pacote[4099] & 0xFF) << 48) |
        ((pacote[4100] & 0xFF) << 40) | ((pacote[4101] & 0xFF) << 32) |
        ((pacote[4102] & 0xFF) << 24) | ((pacote[4103] & 0xFF) << 16) |
        ((pacote[4104] & 0xFF) << 8) | (pacote[4105] & 0xFF);

    pacotes_recebidos[pacote_atual] = numero_sequencia;

    long maior =
        encontrar_maior(pacotes_recebidos, pacote_atual, numero_sequencia);
    long diferenca = maior - numero_sequencia - 1;

    fseek(file, maior * 4096, SEEK_SET);
    fwrite(vetor_zerado, diferenca, 4096, file);

    fseek(file, numero_sequencia * 4096, SEEK_SET);
    fwrite(pacote, 1, 4096, file);

    pacote_atual++;
  }
  printf("Pacotes: %ld\n", pacote_atual);
  printf("done\n");

  fclose(file);
  close(socket_cliente);  // Fecha a conexão socket

  return 0;
}