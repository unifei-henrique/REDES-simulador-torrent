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
  int server_socket, binder, listener, porta;
  struct sockaddr_in serv_addr, cli_addr;
  char nome_arquivo[30];
  socklen_t clilen;
  ssize_t ler_bytes;

  //***************************************************************
  //					  ABERTURA DE CONEXÃO
  //***************************************************************
  server_socket = socket(AF_INET, SOCK_DGRAM, 0);

  if (server_socket <= 0) {
    printf("Erro na abertura do socket: %s\n", strerror(errno));
    exit(1);
  } else if (server_socket) {
    do {
      printf("Aguardando cliente...\n");
    } while (!accept);
  }

  bzero(&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(PORT);

  binder =
      bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (binder < 0) {
    printf("Erro no Bind: %s\n", strerror(errno));
    exit(1);
  }
  clilen = sizeof(cli_addr);
  ler_bytes = recvfrom(server_socket, &nome_arquivo, 20, 0,
                       (struct sockaddr *)&cli_addr, &clilen);

  //***************************************************************
  //					  FIM ABERTURA DE CONEXÃO
  //***************************************************************
  FILE *file;
  int nPacotes = 0, tam_buffer = 4096;
  long long int tamanho_arquivo;
  char str[4096];
  char pacote[4100];

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
    resposta = sendto(server_socket, "Arquivo nao existe", 18, 0,
                      (struct sockaddr *)&cli_addr, sizeof(cli_addr));
  } else {
    resposta = sendto(server_socket, "Arquivo aberto com sucesso", 26, 0,
                      (struct sockaddr *)&cli_addr, sizeof(cli_addr));
  }
  if (resposta <= 0) {
    printf("Erro no read: %s\n", strerror(errno));
    exit(1);
  }

  memset(pacote, 0, sizeof pacote);
  fseek(file, 0, SEEK_END);          // Jump to the end of the file
  tamanho_arquivo = ftell(file);             // Get the current byte offset in the file
  rewind(file); 

  printf("Tamanho do arquivo:%lld\n", tamanho_arquivo);
  while (pacote[4097] == 0) {  // LOOP DE ESCREVER E ENVIAR PACOTES
    fread(pacote, tam_buffer, 1, file);

    pacote[4096] =
        checksum(pacote, tam_buffer);  // Campo de verificação 1 (checksum)
    pacote[4098] = nPacotes; //Campo de verificação 3 (nº de sequencia)

    printf("Tamanho enviado: %d\n", tam_buffer * nPacotes);
    if (tam_buffer * nPacotes + tam_buffer < tamanho_arquivo)
      pacote[4097] = 0;  // Campo de verificação 2 se eh o ultimo pacote

    else                 // Se for ultimo pacote
      pacote[4097] = 1;  // Campo de verificação 2 se eh o ultimo pacote

    while (1) {
      escrever = sendto(server_socket, pacote, sizeof(pacote), 0,
                        (struct sockaddr *)&cli_addr, sizeof(cli_addr));
      if (escrever <= 0)
        printf("Erro ao enviar, tentando novamente\n");
      else {
        nPacotes++;
        break;
      }
    }
  }

  printf("Arquivo enviado com sucesso\n");
  printf("%d\n", nPacotes);

  fclose(file);
  close(server_socket);

  return 0;
}