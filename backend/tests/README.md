# Tests

Folder ini berisi tes untuk modul backend yang dapat diuji secara terpisah.

## Tes room

Menguji create, find, join, penolakan client ketiga, dan remove room.

```bash
make -f makefile.mk test_room
./test_room
```

## Tes crypto

Menguji encrypt/decrypt serta memastikan ciphertext yang diubah ditolak.

```bash
make -f makefile.mk crypto_test
./crypto_test
```

## Tes packet

Menguji pembuatan dan pembongkaran packet serta penolakan packet yang terlalu pendek.

```bash
make -f makefile.mk test_packet
./test_packet
```

## Menjalankan semua tes

```bash
make -f makefile.mk tests
./test_room
./test_packet
./crypto_test
```
