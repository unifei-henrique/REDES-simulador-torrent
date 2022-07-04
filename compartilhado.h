
#define PORT 1337

int checksum(char pacote[], int size) {
  int soma = 0;

  for (int i = 0; i < size; i++) {
    soma += pacote[i];
    soma %= 128;
  }

  return soma;
}