#include <stdio.h>
#include "server.h"


int main(void){
  start_server();
}
        


// #include <stdio.h>
// #include <openssl/rand.h>
// #include <string.h>
// #include "crypto.h"

// #define BUFFER_SIZE 1024


// int main(void) {
//   char input[BUFFER_SIZE];
//   unsigned char key[KEY_SIZE];
//   unsigned char nonce[NONCE_SIZE];
//   unsigned char tag[TAG_SIZE];

//   unsigned char ciphertext[BUFFER_SIZE];
//   unsigned char decrypted[BUFFER_SIZE + 1];

//   printf("Masukkan Pesan\n");

//   if (fgets(input, sizeof(input), stdin) == NULL) {
//     fprintf(stderr, "Gagal membaca input\n");
//     return 1;
// }

// /* Menghapus newline dari fgets() */
// input[strcspn(input, "\n")] = '\0';

// if (RAND_bytes(key, sizeof(key)) != 1 ||
//     RAND_bytes(nonce, sizeof(nonce)) != 1) {
//     fprintf(stderr, "Gagal membuat key atau nonce\n");
//     return 1;
// }

// int ciphertext_len = crypto_encrypt(
//     (const unsigned char *)input,
//     (int)strlen(input),
//     key,
//     nonce,
//     ciphertext,
//     tag
// );

// if (ciphertext_len < 0) {
//     fprintf(stderr, "Enkripsi gagal\n");
//     return 1;
// }

// printf("Ciphertext: ");

// for (int i = 0; i < ciphertext_len; i++) {
//     printf("%02x", ciphertext[i]);
// }

// printf("\n");

// int decrypted_len = crypto_decrypt(
//     ciphertext,
//     ciphertext_len,
//     key,
//     nonce,
//     tag,
//     decrypted
// );

// if (decrypted_len < 0) {
//     fprintf(stderr, "Dekripsi gagal: tag tidak valid\n");
//     return 1;
// }

// /* Tambahkan terminator agar bisa dicetak sebagai string */
// decrypted[decrypted_len] = '\0';

// printf("Hasil dekripsi: %s\n", decrypted);

// return 0;
// }
  

// #include<stdio.h>
// #include"room.h"
// int main (void){
//     unsigned long creator_id;
//     unsigned long joiner_id;

//     char input_code[ROOM_CODE_LENGTH + 1];

//     printf("Masukkan id pembuat room\n");
    
//     if (scanf("%lu", &creator_id) != 1) {
//         printf("ID tidak valid\n");
//         return 1;
//     }

//     Room *room = create_room(creator_id);

//     if (room == NULL) {
//         printf("gagal membuat room\n");
//         return 1;
//     }

//     printf("room berhasil dibuat");
//     printf("room code %s\n", room->code);

//     printf("masukkan client id yg ingin join : ");

//     if (scanf("%lu", &joiner_id) != 1) {
//         printf("kode tidak valid\n");
//         return 1;
//     }

//     printf("Masukkan kode room : ");

//     if (scanf("%6s", input_code) != 1) {
//         printf("kode tidak valid\n");
//         return 1;
//     }

//     Room *joined = join_room(input_code, joiner_id);

//     if (joined == NULL){
//         printf("join gagal: room tidak ditemukan atau penuh\n");
//         return 1;
//     }

//     printf("client %lu berhasil join \n", joiner_id);
//     printf("room %s\n", joined->code);
//     printf("client 1 : %lu\n", joined->client1_id);
//     printf("client 2 : %lu\n", joined->client2_id);

//     return 0;
// }