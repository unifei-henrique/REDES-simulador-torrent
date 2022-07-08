#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
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

int main(int argc, char const *argv[]) {
  int socket_servidor, binder, listener, porta;
  struct sockaddr_in servidor, cliente;
  socklen_t clilen;
  ssize_t ler_bytes;
  char nome_arquivo[30];

  //***************************************************************
  //					  ABERTURA DE CONEXÃO
  //***************************************************************
  socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket_servidor <= 0) {
    printf("Erro na abertura do socket: %s\n", strerror(errno));
    exit(1);
  } else if (socket_servidor) {
    do {
      printf("Aguardando cliente...\n");
    } while (!accept);
  }

  bzero(&servidor, sizeof(servidor));

  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = htonl(INADDR_ANY);
  servidor.sin_port = htons(PORT);

  binder =
      bind(socket_servidor, (struct sockaddr *)&servidor, sizeof(servidor));
  if (binder < 0) {
    printf("Erro no Bind: %s\n", strerror(errno));
    exit(1);
  }
  clilen = sizeof(cliente);
  ler_bytes = recvfrom(socket_servidor, &nome_arquivo, 20, 0,
                       (struct sockaddr *)&cliente, &clilen);

  //***************************************************************
  //					  FIM ABERTURA DE CONEXÃO
  //***************************************************************
  FILE *file;
  long numero_pacotes = 0, pacote_atual = 0;
  int tamanho_buffer = 4096;
  long tamanho_arquivo;
  // char str[4096];
  char pacote[4106];

  ssize_t escrever;
  ssize_t resposta;

  //***************************************************************
  //					   REQUISIÇÃO DE ARQUIVO
  //***************************************************************
  if (ler_bytes <= 0) {
    printf("Erro no read: %s\n", strerror(errno));
    exit(1);
  }

  file = fopen(nome_arquivo, "rb");
  printf("%s\n", nome_arquivo);

  if (!file) {
    resposta = sendto(socket_servidor, "Arquivo nao existe", 18, 0,
                      (struct sockaddr *)&cliente, sizeof(cliente));
  } else {
    resposta = sendto(socket_servidor, "Arquivo aberto com sucesso", 26, 0,
                      (struct sockaddr *)&cliente, sizeof(cliente));
  }
  if (resposta <= 0) {
    printf("Erro no read: %s\n", strerror(errno));
    exit(1);
  }

  memset(pacote, 0, sizeof pacote);
  fseek(file, 0, SEEK_END);       // Jump to the end of the file
  tamanho_arquivo = ftell(file);  // Get the current byte offset in the file
  rewind(file);

  numero_pacotes = (tamanho_arquivo + (tamanho_arquivo % 4096)) / 4096;

  escrever = sendto(socket_servidor, &numero_pacotes, sizeof(long), 0,
                    (struct sockaddr *)&cliente, sizeof(cliente));

  printf("Tamanho do arquivo:%ld\n", tamanho_arquivo);
  while (pacote[4097] == 0) {  // LOOP DE ESCREVER E ENVIAR PACOTES
    fread(pacote, tamanho_buffer, 1, file);

    // Campo de verificação 1 (checksum)
    pacote[4096] = checksum(pacote, tamanho_buffer);

    // Transformando long em char
    pacote[4098] = (pacote_atual >> 56) & 0xFF;
    pacote[4099] = (pacote_atual >> 48) & 0xFF;
    pacote[4100] = (pacote_atual >> 40) & 0xFF;
    pacote[4101] = (pacote_atual >> 32) & 0xFF;
    pacote[4102] = (pacote_atual >> 24) & 0xFF;
    pacote[4103] = (pacote_atual >> 16) & 0xFF;
    pacote[4104] = (pacote_atual >> 8) & 0xFF;
    pacote[4105] = pacote_atual & 0xFF;

    printf("Tamanho enviado: %ld\n", tamanho_buffer * pacote_atual);
    if (numero_pacotes != pacote_atual)
      pacote[4097] = 0;  // Campo de verificação 2 se eh o ultimo pacote
    else
      pacote[4097] = 1;

    while (1) {
      escrever = sendto(socket_servidor, pacote, sizeof(pacote), 0,
                        (struct sockaddr *)&cliente, sizeof(cliente));
      if (escrever <= 0)
        printf("Erro ao enviar, tentando novamente\n");
      else {
        pacote_atual++;
        break;
      }
    }
  }

  printf("Arquivo enviado com sucesso\n");
  printf("%ld\n", numero_pacotes);

  fclose(file);
  close(socket_servidor);

  return 0;
}