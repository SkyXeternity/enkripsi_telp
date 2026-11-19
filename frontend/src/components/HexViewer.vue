<script setup>
import { computed } from 'vue';
import { Copy, Check, Terminal } from 'lucide-vue-next';
import { ref } from 'vue';

const props = defineProps({
  packet: {
    type: Object,
    default: () => null,
  },
});

const copied = ref(false);

const packetStats = computed(() => {
  if (!props.packet) return null;
  return {
    seq: props.packet.seq ?? 0,
    totalBytes: props.packet.totalBytes ?? 0,
    nonce: props.packet.nonceHex || '',
    ciphertext: props.packet.ciphertextHex || '',
    tag: props.packet.tagHex || '',
  };
});

function copyHex() {
  if (!packetStats.value) return;
  const fullHex = `SEQ: ${packetStats.value.seq}\nNONCE (12B): ${packetStats.value.nonce}\nCIPHERTEXT: ${packetStats.value.ciphertext}\nTAG (16B): ${packetStats.value.tag}`;
  navigator.clipboard.writeText(fullHex);
  copied.value = true;
  setTimeout(() => (copied.value = false), 2000);
}
</script>

<template>
  <div class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl">
    <div class="flex items-center justify-between mb-4">
      <div class="flex items-center gap-2">
        <Terminal class="w-5 h-5 text-emerald-400" />
        <h3 class="text-sm font-semibold text-slate-200 uppercase tracking-wider">
          Packet Inspector (Live AES-256-GCM Frame)
        </h3>
      </div>

      <button
        v-if="packetStats"
        @click="copyHex"
        class="inline-flex items-center gap-1.5 px-2.5 py-1 text-xs font-mono bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg transition-colors border border-slate-700"
      >
        <component :is="copied ? Check : Copy" class="w-3.5 h-3.5 text-emerald-400" />
        <span>{{ copied ? 'Tersalin' : 'Salin Hex' }}</span>
      </button>
    </div>

    <!-- No packet placeholder -->
    <div v-if="!packetStats" class="py-12 text-center text-slate-500 text-xs font-mono">
      Menunggu paket transmisi suara atau chat pertama...
    </div>

    <div v-else class="space-y-4">
      <!-- Packet Layout Structure Bar -->
      <div>
        <div class="text-[11px] font-mono text-slate-400 mb-1.5 flex justify-between">
          <span>Struktur Paket Frame (Total: {{ packetStats.totalBytes }} Byte)</span>
          <span class="text-emerald-400 font-semibold">[seq][nonce][ciphertext][tag]</span>
        </div>
        <div class="h-6 w-full rounded-lg overflow-hidden flex text-[10px] font-mono font-bold text-slate-900 text-center leading-6">
          <div class="bg-indigo-400 w-12" title="Sequence Number (4 Byte)">SEQ (4B)</div>
          <div class="bg-cyan-400 w-24" title="Nonce / IV (12 Byte)">NONCE (12B)</div>
          <div class="bg-amber-400 flex-1 truncate px-1" title="Encrypted Payload (Ciphertext)">
            CIPHERTEXT ({{ packetStats.totalBytes - 32 }}B)
          </div>
          <div class="bg-emerald-400 w-24" title="Authentication Tag (16 Byte)">TAG (16B)</div>
        </div>
      </div>

      <!-- Color Legend Details -->
      <div class="grid grid-cols-1 md:grid-cols-2 gap-3 font-mono text-xs">
        <!-- 1. Sequence Number -->
        <div class="bg-slate-950 p-3 rounded-xl border border-indigo-900/40">
          <div class="text-indigo-400 text-[11px] font-semibold flex items-center justify-between mb-1">
            <span>SEQUENCE NUMBER</span>
            <span class="text-[10px] text-slate-500">Offset: 0..3 (4B)</span>
          </div>
          <div class="text-slate-100 font-bold">#{{ packetStats.seq }}</div>
        </div>

        <!-- 2. Nonce / IV -->
        <div class="bg-slate-950 p-3 rounded-xl border border-cyan-900/40">
          <div class="text-cyan-400 text-[11px] font-semibold flex items-center justify-between mb-1">
            <span>GCM NONCE / IV</span>
            <span class="text-[10px] text-slate-500">Offset: 4..15 (12B)</span>
          </div>
          <div class="text-slate-300 break-all text-[11px]">
            {{ packetStats.nonce }}
          </div>
        </div>

        <!-- 3. Ciphertext -->
        <div class="bg-slate-950 p-3 rounded-xl border border-amber-900/40 md:col-span-2">
          <div class="text-amber-400 text-[11px] font-semibold flex items-center justify-between mb-1">
            <span>CIPHERTEXT (ENCRYPTED AUDIO/TEXT)</span>
            <span class="text-[10px] text-slate-500">Offset: 16..{{ packetStats.totalBytes - 16 }}</span>
          </div>
          <div class="text-slate-400 break-all text-[11px] max-h-24 overflow-y-auto font-mono">
            {{ packetStats.ciphertext || '(Kosong)' }}
          </div>
        </div>

        <!-- 4. Authentication Tag -->
        <div class="bg-slate-950 p-3 rounded-xl border border-emerald-900/40 md:col-span-2">
          <div class="text-emerald-400 text-[11px] font-semibold flex items-center justify-between mb-1">
            <span>AUTHENTICATION TAG (GMAC)</span>
            <span class="text-[10px] text-slate-500">Offset: Terakhir (16B)</span>
          </div>
          <div class="text-emerald-300 break-all text-[11px] font-bold">
            {{ packetStats.tag }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>
