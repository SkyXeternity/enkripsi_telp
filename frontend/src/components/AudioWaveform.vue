<script setup>
import { computed } from 'vue';

const props = defineProps({
  label: { type: String, default: 'Audio Stream' },
  volume: { type: Number, default: 0 }, // 0 - 100
  isActive: { type: Boolean, default: false },
  isMuted: { type: Boolean, default: false },
  accentColor: { type: String, default: 'emerald' }, // 'emerald' or 'cyan'
});

// Generate 16 bars with dynamic heights based on volume and sinusoidal fluctuation
const bars = computed(() => {
  const count = 18;
  const arr = [];
  const baseVol = props.isMuted ? 0 : props.volume;

  for (let i = 0; i < count; i++) {
    if (!props.isActive || baseVol <= 2) {
      arr.push(4); // Idle min height
    } else {
      // Dynamic bell curve + noise
      const centerFactor = 1 - Math.abs(i - (count / 2)) / (count / 2);
      const randomJitter = (Math.sin(i * 1.5 + Date.now() / 200) + 1) / 2;
      const height = Math.max(4, Math.round(baseVol * centerFactor * 0.8 + randomJitter * (baseVol * 0.4)));
      arr.push(Math.min(48, height));
    }
  }
  return arr;
});
</script>

<template>
  <div class="bg-slate-900/80 border border-slate-800 rounded-2xl p-4 flex flex-col justify-between shadow-lg">
    <div class="flex items-center justify-between mb-3">
      <div class="flex items-center gap-2">
        <span
          class="w-2.5 h-2.5 rounded-full"
          :class="[
            isMuted ? 'bg-rose-500' :
            isActive && volume > 5 ? (accentColor === 'cyan' ? 'bg-cyan-400 animate-pulse' : 'bg-emerald-400 animate-pulse') :
            isActive ? 'bg-amber-400' : 'bg-slate-600'
          ]"
        ></span>
        <span class="text-xs font-semibold uppercase tracking-wider text-slate-300">{{ label }}</span>
      </div>
      <span
        class="text-[11px] font-mono px-2 py-0.5 rounded-full"
        :class="isMuted ? 'bg-rose-500/20 text-rose-300' : 'bg-slate-800 text-slate-400'"
      >
        {{ isMuted ? 'MUTED' : `${volume}%` }}
      </span>
    </div>

    <!-- Visualizer Waveform Bars -->
    <div class="h-14 flex items-center justify-center gap-1.5 px-2 bg-slate-950/60 rounded-xl border border-slate-800/60">
      <div
        v-for="(h, idx) in bars"
        :key="idx"
        class="w-1.5 rounded-full transition-all duration-75"
        :style="{ height: `${h}px` }"
        :class="[
          isMuted ? 'bg-slate-700' :
          accentColor === 'cyan' ?
            (h > 30 ? 'bg-cyan-300' : h > 15 ? 'bg-cyan-400' : 'bg-cyan-600/60') :
            (h > 30 ? 'bg-emerald-300' : h > 15 ? 'bg-emerald-400' : 'bg-emerald-600/60')
        ]"
      ></div>
    </div>
  </div>
</template>
