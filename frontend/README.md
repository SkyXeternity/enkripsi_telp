# CipherVoice Frontend (Real-time Encrypted Voice Call)

Aplikasi klien web berbasis **Vue 3**, **Vite**, dan **Tailwind CSS** untuk panggilan suara terenkripsi *end-to-end* secara *real-time*. Frontend ini dirancang 100% kompatibel dan tersinkronisasi dengan spesifikasi **Backend C (Mongoose WebSocket + OpenSSL AES-256-GCM)** di folder `../backend`.

---

## 🌟 Fitur Utama

1. **Kompatibilitas Penuh dengan Backend C:**
   - **Format Paket Biner:** `[seq (4B)][nonce (12B)][ciphertext (N B)][tag (16B)]`
   - **Kriptografi:** Web Crypto API `AES-256-GCM` yang identik dengan OpenSSL EVP di `crypto.c`.
   - **Signaling Room:** Kompatibel dengan protokol room (`create_room`, `join_room`, `created`, `peer_joined`) di `room.c` & `urutan.md`.
2. **Audio Engine Real-Time (Web Audio API):**
   - Menangkap suara mikrofon ke format PCM 16-bit berlatensi rendah.
   - Dual Audio Waveform visualizer interaktif (Lokal vs Remote).
   - Pengaturan Mute Mikrofon & Deafen Speaker.
3. **Packet Inspector (Hex Viewer):**
   - Membedah paket live di layar: Sequence Number (Biru), Nonce/IV (Sian), Ciphertext (Amber), dan Tag GMAC (Hijau).
4. **Tamper Lab (Pengujian Tahap 7):**
   - Menguji ketahanan otentikasi AES-GCM terhadap modifikasi data (tamper ciphertext, tamper tag, atau paket abnormal).
5. **Mode Dual (Simulasi / Live C Server):**
   - **Mode Simulasi / Loopback:** Dapat langsung diuji dan didemokan di browser tanpa harus menyalakan backend C terlebih dahulu.
   - **Mode WebSocket C Server:** Menghubungkan langsung ke server Mongoose C (`ws://localhost:8000/websocket`).

---

## 🚀 Cara Menjalankan

### 1. Masuk ke folder frontend dan jalankan:
```bash
npm run dev
```

Buka URL lokal yang muncul di browser (biasanya `http://localhost:5173/`).

### 2. Cara Tes Cepat (Mode Simulasi Loopback):
1. Buka browser di `http://localhost:5173/`.
2. Klik tombol **"Buat Room Baru"**.
3. Klik tombol **"Mulai Panggilan Suara"** dan berikan izin mikrofon.
4. Anda akan melihat waveform suara bergerak dan paket suara live terenkripsi di tab **Packet Inspector**.
5. Buka tab **Tamper Lab** untuk mendemonstrasikan penolakan paket saat data dimodifikasi.

### 3. Cara Menghubungkan ke Backend C:
1. Pastikan server C Mongoose sudah dijalankan di port 8000 (sesuai `urutan.md`).
2. Klik tombol status di header untuk beralih ke **Mode: WebSocket C Server**.
3. Buka tab kedua di browser atau komputer lain pada jaringan yang sama untuk melakukan panggilan 2 arah!
