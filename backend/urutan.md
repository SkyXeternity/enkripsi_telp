# Urutan Kerja Backend (Voice Call Terenkripsi, Bahasa C)

Dikerjakan bertahap, dari yang paling dasar dulu supaya tiap tahap bisa dites sebelum lanjut ke tahap berikutnya.

## Tahap 0: Persiapan

1. Install `gcc`, `make`, `libssl-dev` (OpenSSL), `valgrind`.
2. Download `mongoose.c` dan `mongoose.h` dari repo `cesanta/mongoose`, taruh di folder proyek.
3. Siapkan struktur folder:

```
backend/
├── server.c
├── crypto.c / crypto.h
├── room.c / room.h
├── mongoose.c / mongoose.h
└── Makefile
```

4. Buat `Makefile` sederhana supaya kompilasi tidak perlu diketik ulang tiap kali.

## Tahap 1: Server HTTP/WebSocket Dasar (Tanpa Logika Apa Pun)

1. Ikuti tutorial WebSocket Server dari dokumentasi Mongoose, buat server yang bisa:
   - Menerima koneksi WebSocket di `/ws`
   - Terima pesan teks dari client, kirim balik ("echo")
2. Tes pakai browser (buka console, `new WebSocket(...)`) atau tools seperti `wscat`.
3. **Checkpoint:** server jalan, bisa kirim-terima pesan sederhana.

## Tahap 2: Modul Kriptografi (`crypto.c`)

1. Buat fungsi `encrypt()` dan `decrypt()` pakai OpenSSL AES-256-GCM, **tanpa jaringan dulu**, tes langsung di `main()` sementara:

```c
int main() {
    unsigned char key[32] = {...};
    unsigned char nonce[12] = {...};
    unsigned char *ct = encrypt("halo", key, nonce, ...);
    unsigned char *pt = decrypt(ct, key, nonce, ...);
    printf("%s\n", pt); // harus balik jadi "halo"
}
```

2. Tes kasus gagal: ubah 1 byte ciphertext, pastikan `decrypt()` mendeteksi tag tidak valid.
3. Jalankan dengan `valgrind` untuk pastikan tidak ada memory leak dari `malloc`/`free`.
4. **Checkpoint:** encrypt/decrypt terbukti benar dan aman secara memori, independen dari server.

## Tahap 3: Manajemen Room (`room.c`)

1. Buat struct `Room` dan array/list penyimpanannya di memori.
2. Buat fungsi:
   - `create_room()` → generate kode acak, simpan slot kosong
   - `join_room(code)` → cari room, isi slot kedua, tandai aktif
   - `find_room_by_fd(fd)` → cari room dari koneksi client
   - `remove_room(room)` → hapus room, `memset` kunci ke nol sebelum `free`
3. Tes manual dulu lewat `main()` tanpa jaringan (buat room, join, cek logika penuh/tidak valid).
4. **Checkpoint:** logika room benar sebelum disambungkan ke server.

## Tahap 4: Gabungkan Server + Room (Request Handling)

1. Sambungkan Tahap 1 dan Tahap 3: saat ada pesan `{"type":"create_room"}` atau `{"type":"join_room","code":...}` dari client, panggil fungsi room yang sesuai.
2. Kirim balasan status ke client (`created`, `peer_joined`, `error`).
3. Tes pakai dua tab browser sekaligus: tab A buat room, tab B join pakai kode dari tab A.
4. **Checkpoint:** dua client bisa saling terhubung dalam satu room.

## Tahap 5: Relay Paket (Tanpa Enkripsi Dulu)

1. Tambahkan handling untuk pesan binary: begitu satu client kirim data, server langsung meneruskan (relay) ke client lawan di room yang sama.
2. Tes kirim teks sembarang dari tab A, pastikan muncul di tab B.
3. **Checkpoint:** relay dua arah jalan, server cuma meneruskan tanpa melihat isi (pure relay).

## Tahap 6: Sambungkan Crypto ke Server

1. Definisikan format paket: `[seq][nonce][ciphertext][tag]`.
2. Di sisi client (nanti, saat frontend dibuat) yang akan memanggil encrypt/decrypt; tapi untuk tes backend dulu, buat skrip kecil (bisa pakai Python/Node sementara sebagai "client palsu") yang mengenkripsi teks, kirim ke server, server relay, client lain dekripsi.
3. Pastikan server **tidak decrypt** data itu (server cuma relay byte mentah) → ini yang bikin end-to-end.
4. **Checkpoint:** paket terenkripsi berhasil dikirim A → server → B, dan B bisa dekripsi dengan benar.

## Tahap 7: Uji Ketahanan (Sebelum Dipakai Frontend)

1. Kirim paket dengan tag sengaja diubah → pastikan penerima menolak, bukan crash.
2. Kirim paket terlalu pendek/panjang → pastikan server tidak crash (validasi panjang sebelum `memcpy`).
3. Jalankan server dengan `valgrind`/AddressSanitizer selama pengujian ini.
4. **Checkpoint:** server tahan terhadap input aneh, tidak ada crash atau leak.

## Tahap 8: Multi-client & Concurrency

1. Pastikan server bisa menangani lebih dari satu room sekaligus (event loop Mongoose sudah menangani ini secara default, tapi cek logikamu tidak salah campur antar room).
2. Tes dengan 2 room berjalan bersamaan (4 client total).
3. **Checkpoint:** tidak ada data yang nyasar ke room lain.

## Tahap 9: Integrasi dengan Frontend

Baru di tahap ini backend disambungkan ke UI web (React/Vite) yang sudah dibahas sebelumnya: ganti "client palsu" di Tahap 6 dengan browser asli (Web Audio API + Web Crypto API untuk key exchange dan enkripsi).

---

## Ringkasan Urutan

```
0. Setup proyek & tools
1. Server WebSocket dasar (echo)
2. Modul crypto (standalone, tes lokal)
3. Modul room (standalone, tes lokal)
4. Server + room (request handling create/join)
5. Relay paket mentah antar client
6. Sambungkan crypto ke alur relay
7. Uji ketahanan (input rusak, tag salah)
8. Multi-room/multi-client
9. Integrasi ke frontend web
```

Prinsipnya: **setiap modul dites sendiri-sendiri dulu sebelum digabung**, supaya kalau ada bug, kamu tahu persis di tahap mana masalahnya, bukan harus debug seluruh sistem sekaligus.


const ws = new WebSocket("ws://localhost:8000/websocket");
const ws = new WebSocket(
  "wss://outskirts-equator-capricorn.ngrok-free.dev/websocket"
);
ws.onopen = () => {
  console.log("Terhubung!");
  ws.send("halo server");
};

ws.onmessage = (e) => console.log("Dibalas server:", e.data);
ws.onerror = (e) => console.log("Error:", e);


make -f makefile.mk

---

## Cara Menjalankan Proyek di macOS dan Windows

File `makefile.mk` tetap digunakan pada kedua sistem operasi. Perbedaannya hanya pada instalasi dependency dan nilai lokasi OpenSSL.

### macOS

#### 1. Install dependency

Install Xcode Command Line Tools dan OpenSSL:

```bash
xcode-select --install
brew install openssl
```

#### 2. Clone repository

```bash
git clone <URL_REPOSITORY>
cd call-encrypt.be
```

#### 3. Build dan jalankan

Tes room:

```bash
make -f makefile.mk test_room
./test_room
```

Tes input dari `main.c`:

```bash
make -f makefile.mk main_input
./build/main_input
```

Tes crypto:

```bash
make -f makefile.mk crypto_test
./crypto_test
```

Tes memory leak menggunakan tool bawaan macOS:

```bash
leaks --atExit -- ./test_room
```

### Windows (WSL 2 + Ubuntu)

Proyek ini menggunakan tool Unix/Linux seperti `make`, OpenSSL, dan Valgrind. Untuk pengguna Windows, gunakan WSL 2 dengan Ubuntu.

#### 1. Install WSL dan Ubuntu

Buka PowerShell sebagai Administrator:

```powershell
wsl --install -d Ubuntu
```

Restart Windows jika diminta, lalu buka aplikasi Ubuntu. Panduan resmi: [Microsoft Learn - WSL](https://learn.microsoft.com/windows/wsl/setup/environment).

#### 2. Install dependency di Ubuntu/WSL

Jalankan perintah berikut di terminal Ubuntu, bukan PowerShell:

```bash
sudo apt update
sudo apt install -y build-essential libssl-dev pkg-config valgrind git
```

Dependency yang digunakan:

- `build-essential`: compiler C dan `make`.
- `libssl-dev`: header dan library OpenSSL.
- `pkg-config`: membantu menemukan konfigurasi library.
- `valgrind`: memeriksa memory leak.
- `git`: clone repository.

#### 3. Clone repository di filesystem WSL

```bash
cd ~
git clone <URL_REPOSITORY>
cd call-encrypt.be
```

Sebaiknya simpan repository di home WSL, bukan di `/mnt/c/`, agar proses build lebih cepat.

#### 4. Build dan jalankan

Makefile saat ini menggunakan lokasi OpenSSL dari Homebrew. Di WSL, override lokasinya menjadi `/usr`:

```bash
make -f makefile.mk OPENSSL_PREFIX=/usr test_room
./test_room
```

Untuk mencoba input dari `main.c`:

```bash
make -f makefile.mk OPENSSL_PREFIX=/usr main_input
./build/main_input
```

Untuk target crypto:

```bash
make -f makefile.mk OPENSSL_PREFIX=/usr crypto_test
```

#### 5. Tes memory leak

```bash
valgrind --leak-check=full --show-leak-kinds=all ./test_room
```

Cesanta Mongoose tidak perlu diunduh lagi karena `library/mongoose.c` dan `library/mongoose.h` sudah tersedia di repository. Node.js, npm, dan package npm `mongoose` juga tidak diperlukan untuk backend C saat ini.


- leaks --atExit -- ./crypto_packet_test