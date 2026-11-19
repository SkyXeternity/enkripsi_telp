<script setup>
import { ref, onMounted, onUnmounted, computed, watch } from 'vue';
import {
  Phone,
  PhoneOff,
  Mic,
  MicOff,
  Volume2,
  VolumeX,
  ShieldCheck,
  ShieldAlert,
  Radio,
  Users,
  Copy,
  Check,
  Key,
  Server,
  Activity,
  Layers,
  Bug,
  MessageSquare,
  RefreshCw,
} from 'lucide-vue-next';

import { WebSocketService } from './services/websocketService';
import { AudioEngine } from './services/audioEngine';
import {
  deriveKeyFromPassphrase,
  generateRandomKey,
  importKeyFromHex,
  encryptPacket,
  decryptPacket,
  inspectPacket,
  bytesToHex,
} from './services/cryptoService';

import AudioWaveform from './components/AudioWaveform.vue';
import HexViewer from './components/HexViewer.vue';
import TamperLab from './components/TamperLab.vue';
import EncryptedChat from './components/EncryptedChat.vue';

// -------------------------------------------------------------
// Reactive State
// -------------------------------------------------------------
const ws = new WebSocketService();
const audio = new AudioEngine({ sampleRate: 16000, bufferSize: 1024 });

// Connection & Room
const wsUrl = ref('ws://localhost:8000/websocket');
const wsStatus = ref('DISCONNECTED'); // DISCONNECTED, CONNECTING, CONNECTED, IN_ROOM
const isLoopback = ref(true); // Default loopback agar bisa langsung dites di browser
const roomCodeInput = ref('');
const activeRoomCode = ref('');
const isRoomCreator = ref(false);
const isPeerConnected = ref(false);
const copied = ref(false);

// Call & Audio
const isCallActive = ref(false);
const isMuted = ref(false);
const isDeafened = ref(false);
const localVolume = ref(0);
const remoteVolume = ref(0);
let volumeInterval = null;

// Crypto & Keys
const passphrase = ref('KunciSuaraRahasiaKampus2026');
const activeKeyHex = ref('');
let activeCryptoKey = null;
const keyType = ref('passphrase'); // 'passphrase' or 'raw'
const rawKeyInput = ref('');

// Statistics & Inspection
const seqCounter = ref(0);
const packetsSent = ref(0);
const packetsReceived = ref(0);
const droppedPackets = ref(0);
const lastPacketInfo = ref(null);

// Chat & UI Tabs
const activeTab = ref('inspector'); // 'inspector', 'tamper', 'chat', 'settings'
const chatMessages = ref([]);
const alertMessage = ref(null);

let lastAlertText = '';
let lastAlertTime = 0;

function showAlert(text, type = 'info') {
  const now = Date.now();
  // Cegah spam alert berulang dalam 2.5 detik agar halaman tidak bergetar/stuttering
  if (text === lastAlertText && now - lastAlertTime < 2500) {
    return;
  }
  lastAlertText = text;
  lastAlertTime = now;

  alertMessage.value = { text, type };
  setTimeout(() => {
    if (alertMessage.value?.text === text) alertMessage.value = null;
  }, 4000);
}

// -------------------------------------------------------------
// Key Management
// -------------------------------------------------------------
async function updateEncryptionKey() {
  try {
    if (keyType.value === 'passphrase') {
      const derived = await deriveKeyFromPassphrase(passphrase.value, activeRoomCode.value || 'CipherVoice');
      activeCryptoKey = derived.key;
      activeKeyHex.value = derived.hex;
    } else {
      const imported = await importKeyFromHex(rawKeyInput.value);
      activeCryptoKey = imported.key;
      activeKeyHex.value = imported.hex;
    }
    showAlert('Kunci enkripsi AES-256-GCM berhasil diperbarui', 'success');
  } catch (err) {
    showAlert(`Gagal memproses kunci: ${err.message}`, 'danger');
  }
}

async function handleGenerateRandomKey() {
  const generated = await generateRandomKey();
  keyType.value = 'raw';
  rawKeyInput.value = generated.hex;
  activeCryptoKey = generated.key;
  activeKeyHex.value = generated.hex;
  showAlert('Kunci acak 256-bit baru berhasil dibuat', 'success');
}

// -------------------------------------------------------------
// WebSocket Setup & Room Management
// -------------------------------------------------------------
function setupWebSocket() {
  ws.isLoopbackMode = isLoopback.value;

  ws.onStatusChange = (status) => {
    wsStatus.value = status;
  };

  ws.onRoomCreated = (code) => {
    activeRoomCode.value = code;
    isRoomCreator.value = true;
    updateEncryptionKey();
    showAlert(`Room #${code} berhasil dibuat! (Anda Host)`, 'success');
  };

  ws.onRoomJoined = (code) => {
    activeRoomCode.value = code;
    isRoomCreator.value = false;
    isPeerConnected.value = true;
    updateEncryptionKey();
    showAlert(`Berhasil bergabung ke Room #${code}! (Anda Tamu)`, 'success');
  };

  ws.onPeerJoined = () => {
    isPeerConnected.value = true;
    showAlert('Lawan bicara (Peer) telah bergabung ke room!', 'success');
  };

  ws.onPeerLeft = () => {
    isPeerConnected.value = false;
    showAlert('Lawan bicara telah meninggalkan room', 'danger');
    if (isCallActive.value) endCall();
  };

  ws.onError = (err) => {
    // Abaikan jika server membalas 'request tidak dikenal' (karena server belum memprogram leave handler)
    if (err.message && err.message.toLowerCase().includes('request tidak dikenal')) {
      return;
    }
    showAlert(`Koneksi error: ${err.message || 'Gagal terhubung ke WebSocket'}`, 'danger');
  };

  // Menerima paket biner audio terenkripsi dari peer
  ws.onBinaryReceived = async (buffer) => {
    if (!activeCryptoKey) return;
    packetsReceived.value++;

    // Jika kita adalah Tamu (Guest) dan Host mulai berbicara, otomatis aktifkan suara tamu
    if (!isCallActive.value && activeRoomCode.value && !isRoomCreator.value && isPeerConnected.value) {
      startCall();
    }

    // Dekripsi paket [seq][nonce][ciphertext][tag]
    const decrypted = await decryptPacket(buffer, activeCryptoKey);

    if (decrypted.isValid) {
      // Throttle pembaruan Packet Inspector agar UI tetap ringan 60 FPS
      if (packetsReceived.value % 20 === 0) {
        lastPacketInfo.value = inspectPacket(buffer);
      }
      // Putar audio jika panggilan aktif
      if (isCallActive.value) {
        const pcmArray = new Int16Array(decrypted.plaintext);
        audio.playChunk(pcmArray);
      }
    } else {
      droppedPackets.value++;
      if (droppedPackets.value % 10 === 0) {
        lastPacketInfo.value = inspectPacket(buffer);
      }
      console.warn('Paket ditolak (auth tag mismatch):', decrypted.error);
    }
  };

  // Menerima chat
  ws.onChatMessage = async (msg) => {
    try {
      if (msg.packetHex) {
        const packetBytes = hexToBytes(msg.packetHex);
        const dec = await decryptPacket(packetBytes, activeCryptoKey);
        if (dec.isValid) {
          const text = new TextDecoder().decode(dec.plaintext);
          chatMessages.value.push({
            sender: msg.sender || 'Peer',
            text,
            cipherHex: msg.packetHex.substring(32, 72) + '...',
            time: new Date().toLocaleTimeString(),
            isSelf: false,
          });
        }
      }
    } catch (e) {
      console.error('Gagal dekripsi pesan chat:', e);
    }
  };

  // Mulai koneksi
  ws.connect(wsUrl.value);
}

function handleCreateRoom() {
  isRoomCreator.value = true;
  ws.createRoom();
}

function handleJoinRoom() {
  if (!roomCodeInput.value.trim()) return;
  isRoomCreator.value = false;
  ws.joinRoom(roomCodeInput.value.trim());
}

function handleLeaveRoom() {
  if (isCallActive.value) endCall();
  ws.leaveRoom();
  activeRoomCode.value = '';
  isPeerConnected.value = false;
  isRoomCreator.value = false;
  showAlert('Keluar dari room', 'info');
}

function copyRoomCode() {
  if (!activeRoomCode.value) return;
  navigator.clipboard.writeText(activeRoomCode.value);
  copied.value = true;
  setTimeout(() => (copied.value = false), 2000);
}

// -------------------------------------------------------------
// Voice Call Management
// -------------------------------------------------------------
async function startCall() {
  // Guard: Jangan kirim audio jika di mode server tapi lawan bicara belum ada
  if (!isLoopback.value && !isPeerConnected.value) {
    showAlert('Menunggu lawan bicara bergabung terlebih dahulu', 'danger');
    return;
  }

  try {
    await updateEncryptionKey();
    await audio.startRecording(async (pcmInt16Array) => {
      if (!isCallActive.value || !activeCryptoKey) return;

      seqCounter.value++;
      // Enkripsi PCM Audio ke format [seq][nonce][ciphertext][tag]
      const encryptedPacket = await encryptPacket(
        pcmInt16Array.buffer,
        activeCryptoKey,
        seqCounter.value
      );

      // Kirim via WebSocket relay
      const sent = ws.sendBinary(encryptedPacket);
      if (sent) {
        packetsSent.value++;
        if (packetsSent.value % 20 === 0) {
          lastPacketInfo.value = inspectPacket(encryptedPacket);
        }
      }
    });

    isCallActive.value = true;
    showAlert('Panggilan suara terenkripsi dimulai', 'success');
  } catch (err) {
    showAlert(`Gagal memulai audio: ${err.message}`, 'danger');
  }
}

function endCall() {
  audio.stopRecording();
  isCallActive.value = false;
  localVolume.value = 0;
  remoteVolume.value = 0;
  showAlert('Panggilan suara diakhiri', 'info');
}

function toggleMute() {
  isMuted.value = audio.setMute();
}

function toggleDeafen() {
  isDeafened.value = audio.setDeafen();
}

// -------------------------------------------------------------
// Encrypted Chat Handler
// -------------------------------------------------------------
async function handleSendChat(text) {
  if (!activeCryptoKey) return;
  const enc = new TextEncoder();
  const plaintext = enc.encode(text);
  seqCounter.value++;

  const packet = await encryptPacket(plaintext, activeCryptoKey, seqCounter.value);
  const packetHex = bytesToHex(packet);

  ws.sendChat({
    sender: 'Saya',
    packetHex,
  });

  chatMessages.value.push({
    sender: 'Saya',
    text,
    cipherHex: packetHex.substring(32, 72) + '...',
    time: new Date().toLocaleTimeString(),
    isSelf: true,
  });
}

function toggleMode() {
  isLoopback.value = !isLoopback.value;
  ws.disconnect();
  setupWebSocket();
}

// Volume polling
onMounted(async () => {
  await updateEncryptionKey();
  setupWebSocket();

  volumeInterval = setInterval(() => {
    localVolume.value = audio.localVolume;
    remoteVolume.value = audio.remoteVolume;
  }, 50);
});

onUnmounted(() => {
  if (volumeInterval) clearInterval(volumeInterval);
  audio.close();
  ws.disconnect();
});
</script>

<template>
  <div class="min-h-screen bg-slate-950 text-slate-100 flex flex-col font-sans selection:bg-emerald-500 selection:text-slate-950">
    <!-- Top Header -->
    <header class="border-b border-slate-800/80 bg-slate-900/60 backdrop-blur sticky top-0 z-30 px-4 lg:px-8 py-3.5">
      <div class="max-w-7xl mx-auto flex flex-wrap items-center justify-between gap-3">
        <!-- Logo & Branding -->
        <div class="flex items-center gap-3">
          <div class="w-10 h-10 rounded-xl bg-gradient-to-tr from-emerald-600 to-cyan-500 flex items-center justify-center shadow-lg shadow-emerald-500/20">
            <Radio class="w-5 h-5 text-slate-950" />
          </div>
          <div>
            <div class="flex items-center gap-2">
              <h1 class="text-base font-bold tracking-tight text-white">CipherVoice</h1>
              <span class="text-[10px] uppercase font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400 border border-emerald-500/30 font-semibold">
                AP3 Kampus
              </span>
            </div>
            <p class="text-xs text-slate-400">Real-Time Encrypted Voice Call (AES-256-GCM + Mongoose C)</p>
          </div>
        </div>

        <!-- Badges & Mode Switcher -->
        <div class="flex flex-wrap items-center gap-2 text-xs">
          <!-- Backend / WS Mode Toggle -->
          <button
            @click="toggleMode"
            class="flex items-center gap-1.5 px-3 py-1.5 rounded-xl border transition cursor-pointer"
            :class="isLoopback ? 'bg-indigo-950/60 border-indigo-700/60 text-indigo-300' : 'bg-slate-800 border-slate-700 text-slate-300'"
            title="Klik untuk beralih antara Mode Simulasi Loopback dan Server C Mongoose"
          >
            <Server class="w-3.5 h-3.5" />
            <span class="font-medium">{{ isLoopback ? 'Mode: Simulasi / Loopback' : 'Mode: WebSocket C Server' }}</span>
          </button>

          <!-- Status Connection Badge -->
          <div
            class="flex items-center gap-1.5 px-3 py-1.5 rounded-xl border"
            :class="[
              wsStatus === 'IN_ROOM' ? 'bg-emerald-950/60 border-emerald-700/60 text-emerald-300' :
              wsStatus === 'CONNECTED' ? 'bg-cyan-950/60 border-cyan-700/60 text-cyan-300' :
              wsStatus === 'CONNECTING' ? 'bg-amber-950/60 border-amber-700/60 text-amber-300' :
              'bg-slate-900 border-slate-800 text-slate-400'
            ]"
          >
            <span
              class="w-2 h-2 rounded-full"
              :class="[
                wsStatus === 'IN_ROOM' ? 'bg-emerald-400 animate-ping' :
                wsStatus === 'CONNECTED' ? 'bg-cyan-400' :
                wsStatus === 'CONNECTING' ? 'bg-amber-400 animate-pulse' :
                'bg-slate-600'
              ]"
            ></span>
            <span class="font-mono text-[11px]">{{ wsStatus }}</span>
          </div>

          <!-- Encryption Badge -->
          <div class="flex items-center gap-1 px-3 py-1.5 rounded-xl bg-slate-900 border border-slate-800 text-slate-300 font-mono text-[11px]">
            <ShieldCheck class="w-3.5 h-3.5 text-emerald-400" />
            <span>E2E AES-256</span>
          </div>
        </div>
      </div>
    </header>

    <!-- Global Toast Alert -->
    <div v-if="alertMessage" class="max-w-7xl mx-auto w-full px-4 lg:px-8 mt-3">
      <div
        class="px-4 py-2.5 rounded-xl border text-xs font-medium flex items-center justify-between transition shadow-md"
        :class="[
          alertMessage.type === 'success' ? 'bg-emerald-950/80 border-emerald-600/60 text-emerald-200' :
          alertMessage.type === 'danger' ? 'bg-rose-950/80 border-rose-600/60 text-rose-200' :
          'bg-slate-900 border-slate-700 text-slate-200'
        ]"
      >
        <span>{{ alertMessage.text }}</span>
        <button @click="alertMessage = null" class="text-xs opacity-70 hover:opacity-100 ml-4 font-mono">✕</button>
      </div>
    </div>

    <!-- Main Container -->
    <main class="max-w-7xl mx-auto w-full px-4 lg:px-8 py-6 flex-1 grid grid-cols-1 lg:grid-cols-12 gap-6">

      <!-- Left Column: Room & Call Controls (5 Cols) -->
      <div class="lg:col-span-5 space-y-6">

        <!-- Card 1: Room Management -->
        <div class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl space-y-4">
          <div class="flex items-center justify-between">
            <h2 class="text-sm font-semibold text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <Users class="w-4 h-4 text-emerald-400" />
              Room Komunikasi
            </h2>
            <span v-if="activeRoomCode" class="text-xs font-mono px-2 py-0.5 rounded border flex items-center gap-1.5"
              :class="isRoomCreator ? 'text-emerald-300 bg-emerald-950/80 border-emerald-700/60' : 'text-cyan-300 bg-cyan-950/80 border-cyan-700/60'">
              <span class="w-1.5 h-1.5 rounded-full" :class="isRoomCreator ? 'bg-emerald-400' : 'bg-cyan-400'"></span>
              {{ isRoomCreator ? 'HOST' : 'GUEST' }}: #{{ activeRoomCode }}
            </span>
          </div>

          <!-- When NOT in room -->
          <div v-if="!activeRoomCode" class="space-y-3">
            <button
              @click="handleCreateRoom"
              class="w-full bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-bold py-2.5 px-4 rounded-xl text-sm transition flex items-center justify-center gap-2 shadow-lg shadow-emerald-600/20"
            >
              <Radio class="w-4 h-4" />
              <span>Buat Room Baru (6 Digit)</span>
            </button>

            <div class="relative flex py-1 items-center">
              <div class="flex-grow border-t border-slate-800"></div>
              <span class="flex-shrink mx-3 text-[11px] text-slate-500 font-mono">atau gabung kode</span>
              <div class="flex-grow border-t border-slate-800"></div>
            </div>

            <div class="flex gap-2">
              <input
                v-model="roomCodeInput"
                type="text"
                placeholder="Contoh: 000001"
                maxlength="6"
                class="flex-1 bg-slate-950 border border-slate-700 rounded-xl px-3.5 py-2 text-sm text-slate-200 font-mono tracking-widest text-center focus:outline-none focus:border-emerald-500"
              />
              <button
                @click="handleJoinRoom"
                :disabled="!roomCodeInput.trim()"
                class="bg-slate-800 hover:bg-slate-700 disabled:opacity-50 text-slate-200 px-4 py-2 rounded-xl text-xs font-semibold border border-slate-700 transition"
              >
                Gabung
              </button>
            </div>
          </div>

          <!-- When in Room -->
          <div v-else class="space-y-4">
            <div class="bg-slate-950 p-4 rounded-xl border border-slate-800 flex items-center justify-between">
              <div>
                <div class="text-[11px] text-slate-500 font-mono">KODE ROOM ANDA</div>
                <div class="text-2xl font-mono font-bold text-emerald-400 tracking-wider">
                  {{ activeRoomCode }}
                </div>
              </div>
              <button
                @click="copyRoomCode"
                class="p-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition"
                title="Salin Kode Room"
              >
                <component :is="copied ? Check : Copy" class="w-4 h-4 text-emerald-400" />
              </button>
            </div>

            <div class="flex items-center justify-between text-xs px-1">
              <span class="text-slate-400 flex items-center gap-1.5">
                <span class="w-2 h-2 rounded-full" :class="isPeerConnected ? 'bg-emerald-400' : 'bg-amber-400 animate-ping'"></span>
                {{ isPeerConnected ? 'Peer Terhubung' : 'Menunggu Lawan Bicara...' }}
              </span>
              <button
                @click="handleLeaveRoom"
                class="text-rose-400 hover:text-rose-300 text-xs font-semibold"
              >
                Tinggalkan Room
              </button>
            </div>
          </div>
        </div>

        <!-- Card 2: Voice Call Hub -->
        <div class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl space-y-5">
          <div class="flex items-center justify-between">
            <h2 class="text-sm font-semibold text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <Phone class="w-4 h-4 text-cyan-400" />
              Voice Call Controls
            </h2>
            <span
              class="text-[11px] font-mono px-2 py-0.5 rounded-full"
              :class="isCallActive ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/40' : 'bg-slate-800 text-slate-500'"
            >
              {{ isCallActive ? 'CALL ACTIVE' : 'CALL STANDBY' }}
            </span>
          </div>

          <!-- Dual Audio Waveforms -->
          <div class="grid grid-cols-1 sm:grid-cols-2 gap-3">
            <AudioWaveform
              label="Suara Anda (Local Mic)"
              :volume="localVolume"
              :is-active="isCallActive"
              :is-muted="isMuted"
              accent-color="emerald"
            />
            <AudioWaveform
              label="Lawan Bicara (Remote)"
              :volume="remoteVolume"
              :is-active="isCallActive && isPeerConnected"
              :is-muted="isDeafened"
              accent-color="cyan"
            />
          </div>

          <!-- Primary Call Action Buttons -->
          <div class="flex items-center justify-center gap-3 pt-2">
            <!-- Start / End Call -->
            <button
              v-if="!isCallActive"
              @click="startCall"
              :disabled="!activeRoomCode || (!isLoopback && isRoomCreator && !isPeerConnected) || (!isLoopback && !isRoomCreator)"
              class="flex-1 font-bold py-3 px-4 rounded-xl text-sm transition flex items-center justify-center gap-2 shadow-lg"
              :class="[
                !activeRoomCode ? 'bg-slate-800 text-slate-500 cursor-not-allowed opacity-50' :
                (!isLoopback && isRoomCreator && !isPeerConnected) ? 'bg-amber-950/60 text-amber-300 border border-amber-600/50 cursor-not-allowed opacity-80' :
                (!isLoopback && !isRoomCreator) ? 'bg-slate-800 text-cyan-300 border border-cyan-800/60 cursor-not-allowed opacity-80' :
                'bg-emerald-600 hover:bg-emerald-500 text-slate-950 shadow-emerald-600/30'
              ]"
            >
              <Phone class="w-4 h-4" />
              <span v-if="!activeRoomCode">Pilih / Buat Room Terlebih Dahulu</span>
              <span v-else-if="!isLoopback && isRoomCreator && !isPeerConnected">Menunggu Lawan Bicara Join...</span>
              <span v-else-if="!isLoopback && !isRoomCreator">Menunggu Host Memulai Panggilan...</span>
              <span v-else-if="isRoomCreator">Mulai Panggilan Suara (Host)</span>
              <span v-else>Mulai Panggilan Suara</span>
            </button>

            <button
              v-else
              @click="endCall"
              class="flex-1 bg-rose-600 hover:bg-rose-500 text-white font-bold py-3 px-4 rounded-xl text-sm transition flex items-center justify-center gap-2 shadow-lg shadow-rose-600/30"
            >
              <PhoneOff class="w-4 h-4" />
              <span>Akhiri Panggilan</span>
            </button>

            <!-- Mute Mic -->
            <button
              @click="toggleMute"
              :disabled="!isCallActive"
              class="p-3 rounded-xl border transition"
              :class="[
                isMuted ? 'bg-rose-600/30 border-rose-500 text-rose-300' :
                'bg-slate-800 hover:bg-slate-700 border-slate-700 text-slate-300 disabled:opacity-40'
              ]"
              title="Mute / Unmute Mikrofon"
            >
              <component :is="isMuted ? MicOff : Mic" class="w-4 h-4" />
            </button>

            <!-- Deafen Speaker -->
            <button
              @click="toggleDeafen"
              :disabled="!isCallActive"
              class="p-3 rounded-xl border transition"
              :class="[
                isDeafened ? 'bg-rose-600/30 border-rose-500 text-rose-300' :
                'bg-slate-800 hover:bg-slate-700 border-slate-700 text-slate-300 disabled:opacity-40'
              ]"
              title="Deafen / Matikan Speaker"
            >
              <component :is="isDeafened ? VolumeX : Volume2" class="w-4 h-4" />
            </button>
          </div>

          <!-- Quick Stats Row -->
          <div class="grid grid-cols-3 gap-2 pt-2 border-t border-slate-800 text-center font-mono text-[11px]">
            <div class="bg-slate-950/60 p-2 rounded-lg border border-slate-800/80">
              <div class="text-slate-500 text-[10px]">TERKIRIM</div>
              <div class="text-emerald-400 font-bold">{{ packetsSent }} pkt</div>
            </div>
            <div class="bg-slate-950/60 p-2 rounded-lg border border-slate-800/80">
              <div class="text-slate-500 text-[10px]">DITERIMA</div>
              <div class="text-cyan-400 font-bold">{{ packetsReceived }} pkt</div>
            </div>
            <div class="bg-slate-950/60 p-2 rounded-lg border border-slate-800/80">
              <div class="text-slate-500 text-[10px]">DITOLAK/DROP</div>
              <div class="text-rose-400 font-bold">{{ droppedPackets }} pkt</div>
            </div>
          </div>
        </div>
      </div>

      <!-- Right Column: Tabs (Inspector, Tamper Lab, Chat, Settings) (7 Cols) -->
      <div class="lg:col-span-7 space-y-4">
        <!-- Navigation Tabs -->
        <div class="flex items-center gap-1.5 bg-slate-900 border border-slate-800 p-1.5 rounded-2xl overflow-x-auto text-xs">
          <button
            @click="activeTab = 'inspector'"
            class="flex items-center gap-1.5 px-3.5 py-2 rounded-xl transition whitespace-nowrap font-medium"
            :class="activeTab === 'inspector' ? 'bg-emerald-600 text-slate-950 font-bold' : 'text-slate-400 hover:text-slate-200'"
          >
            <Layers class="w-3.5 h-3.5" />
            <span>Packet Inspector</span>
          </button>

          <button
            @click="activeTab = 'tamper'"
            class="flex items-center gap-1.5 px-3.5 py-2 rounded-xl transition whitespace-nowrap font-medium"
            :class="activeTab === 'tamper' ? 'bg-amber-500 text-slate-950 font-bold' : 'text-slate-400 hover:text-slate-200'"
          >
            <Bug class="w-3.5 h-3.5" />
            <span>Tamper Lab (Tahap 7)</span>
          </button>

          <!-- <button
            @click="activeTab = 'chat'"
            class="flex items-center gap-1.5 px-3.5 py-2 rounded-xl transition whitespace-nowrap font-medium"
            :class="activeTab === 'chat' ? 'bg-cyan-500 text-slate-950 font-bold' : 'text-slate-400 hover:text-slate-200'"
          >
            <MessageSquare class="w-3.5 h-3.5" />
            <span>E2E Chat</span>
          </button> -->

          <button
            @click="activeTab = 'settings'"
            class="flex items-center gap-1.5 px-3.5 py-2 rounded-xl transition whitespace-nowrap font-medium"
            :class="activeTab === 'settings' ? 'bg-slate-700 text-white font-bold' : 'text-slate-400 hover:text-slate-200'"
          >
            <Key class="w-3.5 h-3.5" />
            <span>Kunci & Konfigurasi</span>
          </button>
        </div>

        <!-- Tab 1: Packet Inspector -->
        <div v-show="activeTab === 'inspector'">
          <HexViewer :packet="lastPacketInfo" />
        </div>

        <!-- Tab 2: Tamper Lab -->
        <div v-show="activeTab === 'tamper'">
          <TamperLab />
        </div>

        <!-- Tab 3: Encrypted Chat -->
        <div v-show="activeTab === 'chat'">
          <EncryptedChat
            :messages="chatMessages"
            :is-connected="!!activeRoomCode"
            @send-message="handleSendChat"
          />
        </div>

        <!-- Tab 4: Settings -->
        <div v-show="activeTab === 'settings'" class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl space-y-5 text-xs">
          <div class="border-b border-slate-800 pb-3">
            <h3 class="text-sm font-semibold text-slate-200 uppercase tracking-wider flex items-center gap-2">
              <Key class="w-4 h-4 text-emerald-400" />
              Pengaturan Kunci Enkripsi End-to-End
            </h3>
            <p class="text-slate-400 text-[11px] mt-0.5">
              Kedua klien harus menggunakan kunci atau passphrase yang sama agar komunikasi suara dapat didekripsi.
            </p>
          </div>

          <!-- Key Mode -->
          <div class="space-y-3">
            <div class="flex gap-4">
              <label class="flex items-center gap-2 cursor-pointer">
                <input type="radio" value="passphrase" v-model="keyType" @change="updateEncryptionKey" />
                <span>Passphrase / Kata Sandi Room (PBKDF2)</span>
              </label>
              <label class="flex items-center gap-2 cursor-pointer">
                <input type="radio" value="raw" v-model="keyType" @change="updateEncryptionKey" />
                <span>Raw 256-bit Hex Key</span>
              </label>
            </div>

            <!-- Passphrase input -->
            <div v-if="keyType === 'passphrase'" class="space-y-1.5">
              <label class="text-slate-400 font-mono text-[11px]">Passphrase Bersama:</label>
              <div class="flex gap-2">
                <input
                  v-model="passphrase"
                  type="text"
                  class="flex-1 bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-slate-200 font-mono focus:outline-none focus:border-emerald-500"
                />
                <button
                  @click="updateEncryptionKey"
                  class="bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-semibold px-3 py-2 rounded-xl transition"
                >
                  Terapkan
                </button>
              </div>
            </div>

            <!-- Raw Hex Key input -->
            <div v-else class="space-y-2">
              <label class="text-slate-400 font-mono text-[11px]">Kunci Hex (64 Karakter):</label>
              <input
                v-model="rawKeyInput"
                type="text"
                placeholder="64 hex characters (256-bit)"
                class="w-full bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-slate-200 font-mono text-[11px] focus:outline-none focus:border-emerald-500"
              />
              <div class="flex gap-2">
                <button
                  @click="updateEncryptionKey"
                  class="bg-emerald-600 hover:bg-emerald-500 text-slate-950 font-semibold px-3 py-1.5 rounded-xl transition"
                >
                  Terapkan Hex
                </button>
                <button
                  @click="handleGenerateRandomKey"
                  class="bg-slate-800 hover:bg-slate-700 text-slate-200 px-3 py-1.5 rounded-xl border border-slate-700 transition"
                >
                  Generate Kunci Acak Baru
                </button>
              </div>
            </div>

            <!-- Active Key Display -->
            <div class="bg-slate-950 p-3 rounded-xl border border-slate-800 font-mono">
              <div class="text-[10px] text-slate-500 uppercase tracking-wider mb-1">
                KUNCI AKTIF DI MEMORI (AES-256):
              </div>
              <div class="text-emerald-400 break-all text-[11px]">
                {{ activeKeyHex }}
              </div>
            </div>

            <!-- WebSocket URL Configuration -->
            <div class="pt-3 border-t border-slate-800 space-y-1.5">
              <label class="text-slate-400 font-mono text-[11px]">Endpoint WebSocket Backend C:</label>
              <div class="flex gap-2">
                <input
                  v-model="wsUrl"
                  type="text"
                  class="flex-1 bg-slate-950 border border-slate-700 rounded-xl px-3 py-2 text-slate-200 font-mono focus:outline-none focus:border-cyan-500"
                />
                <button
                  @click="setupWebSocket"
                  class="bg-slate-800 hover:bg-slate-700 text-slate-200 font-semibold px-3 py-2 rounded-xl border border-slate-700 transition"
                >
                  Reconnect
                </button>
              </div>
              <p class="text-[10px] text-slate-500">
                Default: <code>ws://localhost:8000/websocket</code> (sesuai backend Mongoose C).
              </p>
            </div>
          </div>
        </div>
      </div>
    </main>
  </div>
</template>
