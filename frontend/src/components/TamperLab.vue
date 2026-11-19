<script setup>
import { ref } from 'vue';
import { ShieldAlert, ShieldCheck, Bug, Play, RefreshCw, Lock } from 'lucide-vue-next';
import {
  encryptPacket,
  decryptPacket,
  tamperPacket,
  inspectPacket,
  deriveKeyFromPassphrase,
} from '../services/cryptoService';

const testInput = ref('Ini adalah sampel paket suara rahasia');
const logs = ref([]);
const isRunning = ref(false);

function addLog(type, title, details) {
  logs.value.unshift({
    id: Date.now() + Math.random(),
    time: new Date().toLocaleTimeString(),
    type, // 'success', 'danger', 'info'
    title,
    details,
  });
  if (logs.value.length > 20) logs.value.pop();
}

async function runTest(testType) {
  isRunning.value = true;
  try {
    // 1. Siapkan Kunci AES-256
    const { key } = await deriveKeyFromPassphrase('TamperLabSecretKey');
    const enc = new TextEncoder();
    const plaintext = enc.encode(testInput.value);

    // 2. Enkripsi paket normal: [seq][nonce][ciphertext][tag]
    const originalPacket = await encryptPacket(plaintext, key, Math.floor(Math.random() * 1000));
    const originalInfo = inspectPacket(originalPacket);

    let testPacket = originalPacket;
    let description = '';

    if (testType === 'normal') {
      description = 'Menguji transmisi paket murni tanpa modifikasi apa pun.';
    } else if (testType === 'tamper-cipher') {
      testPacket = tamperPacket(originalPacket, 'ciphertext');
      description = 'Serangan Man-in-the-Middle: 1 byte pada ciphertext dimanipulasi!';
    } else if (testType === 'tamper-tag') {
      testPacket = tamperPacket(originalPacket, 'tag');
      description = 'Serangan Pemalsuan Tag: 1 byte pada authentication tag diubah!';
    } else if (testType === 'short-packet') {
      testPacket = originalPacket.slice(0, 14); // Di bawah 32 byte
      description = 'Uji Ketahanan: Mengirim paket abnormal yang terpotong (< 32 byte).';
    }

    // 3. Dekripsi paket hasil pengujian
    const result = await decryptPacket(testPacket, key);

    if (result.isValid) {
      const dec = new TextDecoder();
      const decryptedText = dec.decode(result.plaintext);
      addLog(
        'success',
        `[${testType.toUpperCase()}] Dekripsi Berhasil (Tag Autentik Valid)`,
        `${description}\nPlaintext hasil dekripsi: "${decryptedText}"`
      );
    } else {
      addLog(
        'danger',
        `[${testType.toUpperCase()}] DITOLAK: Integritas Rusak!`,
        `${description}\nRespon Sistem: ${result.error}\n(Paket dibuang secara aman tanpa crash, membuktikan proteksi AES-GCM berfungsi)`
      );
    }
  } catch (err) {
    addLog('danger', 'Error Eksekusi Uji', err.message);
  } finally {
    isRunning.value = false;
  }
}

function clearLogs() {
  logs.value = [];
}
</script>

<template>
  <div class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl space-y-5">
    <div class="flex items-center justify-between border-b border-slate-800 pb-3">
      <div class="flex items-center gap-2">
        <Bug class="w-5 h-5 text-amber-400" />
        <div>
          <h3 class="text-sm font-semibold text-slate-200 uppercase tracking-wider">
            Tamper & Cryptographic Robustness Lab (Tahap 7)
          </h3>
          <p class="text-xs text-slate-400">
            Simulasikan serangan modifikasi data jaringan untuk membuktikan ketahanan otentikasi AES-GCM.
          </p>
        </div>
      </div>
      <button
        @click="clearLogs"
        class="text-xs text-slate-400 hover:text-slate-200 px-2.5 py-1 rounded bg-slate-800 border border-slate-700"
      >
        Bersihkan Log
      </button>
    </div>

    <!-- Test Input -->
    <div>
      <label class="block text-xs font-medium text-slate-300 mb-1.5">
        Sampel Plaintext Payload
      </label>
      <input
        v-model="testInput"
        type="text"
        class="w-full bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-sm text-slate-200 font-mono focus:outline-none focus:border-emerald-500"
      />
    </div>

    <!-- Action Buttons -->
    <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-2.5">
      <!-- 1. Normal Test -->
      <button
        @click="runTest('normal')"
        :disabled="isRunning"
        class="flex items-center justify-center gap-2 px-3 py-2.5 bg-emerald-600/20 hover:bg-emerald-600/30 text-emerald-300 border border-emerald-500/40 rounded-xl text-xs font-semibold transition"
      >
        <ShieldCheck class="w-4 h-4 text-emerald-400" />
        <span>Uji Paket Normal</span>
      </button>

      <!-- 2. Tamper Ciphertext -->
      <button
        @click="runTest('tamper-cipher')"
        :disabled="isRunning"
        class="flex items-center justify-center gap-2 px-3 py-2.5 bg-amber-600/20 hover:bg-amber-600/30 text-amber-300 border border-amber-500/40 rounded-xl text-xs font-semibold transition"
      >
        <Bug class="w-4 h-4 text-amber-400" />
        <span>Tamper Ciphertext</span>
      </button>

      <!-- 3. Tamper Tag -->
      <button
        @click="runTest('tamper-tag')"
        :disabled="isRunning"
        class="flex items-center justify-center gap-2 px-3 py-2.5 bg-rose-600/20 hover:bg-rose-600/30 text-rose-300 border border-rose-500/40 rounded-xl text-xs font-semibold transition"
      >
        <ShieldAlert class="w-4 h-4 text-rose-400" />
        <span>Tamper Auth Tag</span>
      </button>

      <!-- 4. Abnormal Length -->
      <button
        @click="runTest('short-packet')"
        :disabled="isRunning"
        class="flex items-center justify-center gap-2 px-3 py-2.5 bg-indigo-600/20 hover:bg-indigo-600/30 text-indigo-300 border border-indigo-500/40 rounded-xl text-xs font-semibold transition"
      >
        <Lock class="w-4 h-4 text-indigo-400" />
        <span>Paket Terpotong (&lt;32B)</span>
      </button>
    </div>

    <!-- Log Console -->
    <div class="bg-slate-950 border border-slate-800 rounded-xl p-3 max-h-56 overflow-y-auto font-mono text-xs space-y-2">
      <div v-if="logs.length === 0" class="text-slate-500 text-center py-4 text-[11px]">
        Klik salah satu tombol uji di atas untuk melihat respon validasi keamanan.
      </div>
      <div
        v-for="log in logs"
        :key="log.id"
        class="p-2.5 rounded-lg border leading-relaxed"
        :class="[
          log.type === 'success' ? 'bg-emerald-950/40 border-emerald-800/60 text-emerald-300' :
          log.type === 'danger' ? 'bg-rose-950/40 border-rose-800/60 text-rose-300' :
          'bg-slate-900 border-slate-800 text-slate-300'
        ]"
      >
        <div class="flex items-center justify-between text-[10px] opacity-75 mb-1">
          <span class="font-bold">{{ log.title }}</span>
          <span>{{ log.time }}</span>
        </div>
        <div class="whitespace-pre-line text-[11px] opacity-90">{{ log.details }}</div>
      </div>
    </div>
  </div>
</template>
