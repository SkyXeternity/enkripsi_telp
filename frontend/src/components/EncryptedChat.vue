<script setup>
import { ref } from 'vue';
import { Send, Lock, ShieldCheck } from 'lucide-vue-next';

const props = defineProps({
  messages: {
    type: Array,
    default: () => [],
  },
  isConnected: {
    type: Boolean,
    default: false,
  },
});

const emit = defineEmits(['send-message']);

const inputText = ref('');

function submit() {
  if (!inputText.value.trim() || !props.isConnected) return;
  emit('send-message', inputText.value.trim());
  inputText.value = '';
}
</script>

<template>
  <div class="bg-slate-900 border border-slate-800 rounded-2xl p-5 shadow-xl flex flex-col h-[420px]">
    <div class="flex items-center justify-between border-b border-slate-800 pb-3 mb-3">
      <div class="flex items-center gap-2">
        <Lock class="w-4 h-4 text-emerald-400" />
        <h3 class="text-sm font-semibold text-slate-200 uppercase tracking-wider">
          E2E Encrypted Room Chat
        </h3>
      </div>
      <span class="text-[11px] font-mono text-emerald-400 flex items-center gap-1">
        <ShieldCheck class="w-3.5 h-3.5" />
        AES-256-GCM
      </span>
    </div>

    <!-- Messages List -->
    <div class="flex-1 overflow-y-auto space-y-3 pr-1 text-xs">
      <div v-if="messages.length === 0" class="h-full flex items-center justify-center text-slate-500 font-mono text-center">
        Belum ada pesan. Ketik pesan di bawah untuk menguji transmisi teks terenkripsi.
      </div>

      <div
        v-for="(msg, i) in messages"
        :key="i"
        class="flex flex-col"
        :class="msg.isSelf ? 'items-end' : 'items-start'"
      >
        <div class="text-[10px] text-slate-500 mb-0.5">
          {{ msg.sender }} • {{ msg.time }}
        </div>
        <div
          class="max-w-[85%] rounded-2xl px-3.5 py-2 shadow"
          :class="msg.isSelf ? 'bg-emerald-600/30 border border-emerald-500/50 text-emerald-100 rounded-br-none' : 'bg-slate-800 border border-slate-700 text-slate-200 rounded-bl-none'"
        >
          <div class="font-sans text-sm">{{ msg.text }}</div>
          <!-- Hex Preview in Chat -->
          <div v-if="msg.cipherHex" class="mt-1.5 pt-1 border-t border-slate-700/50 text-[10px] font-mono opacity-70 truncate max-w-xs">
            <span class="text-amber-400">CIPHER:</span> {{ msg.cipherHex }}
          </div>
        </div>
      </div>
    </div>

    <!-- Input Form -->
    <form @submit.prevent="submit" class="mt-3 pt-3 border-t border-slate-800 flex gap-2">
      <input
        v-model="inputText"
        :disabled="!isConnected"
        type="text"
        placeholder="Ketik pesan rahasia..."
        class="flex-1 bg-slate-950 border border-slate-700 rounded-xl px-3.5 py-2 text-sm text-slate-200 focus:outline-none focus:border-emerald-500 disabled:opacity-50"
      />
      <button
        type="submit"
        :disabled="!isConnected || !inputText.trim()"
        class="bg-emerald-600 hover:bg-emerald-500 disabled:opacity-50 text-slate-950 px-4 py-2 rounded-xl font-semibold flex items-center gap-1.5 transition"
      >
        <Send class="w-4 h-4" />
        <span class="text-xs">Kirim</span>
      </button>
    </form>
  </div>
</template>
