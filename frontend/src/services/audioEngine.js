/**
 * CipherVoice Real-Time Audio Engine
 * Menggunakan Web Audio API untuk menangkap suara mikrofon (PCM 16-bit),
 * menghitung RMS untuk audio visualizer, serta memutar audio streaming yang didekripsi.
 */

export class AudioEngine {
  constructor(options = {}) {
    this.sampleRate = options.sampleRate || 16000;
    this.bufferSize = options.bufferSize || 1024; // ~64ms pada 16kHz
    this.audioContext = null;
    this.mediaStream = null;
    this.micSource = null;
    this.processorNode = null;
    this.gainNode = null;

    this.isRecording = false;
    this.isMuted = false;
    this.isDeafened = false;

    this.localVolume = 0;
    this.remoteVolume = 0;

    this.onAudioChunk = null; // Callback saat ada data audio mikrofon siap dienkripsi

    // Playback scheduling & jitter buffer
    this.nextStartTime = 0;
    this.playbackGain = null;
  }

  /**
   * Inisialisasi AudioContext jika belum aktif
   */
  async ensureAudioContext() {
    if (!this.audioContext || this.audioContext.state === 'closed') {
      const AudioCtx = window.AudioContext || window.webkitAudioContext;
      this.audioContext = new AudioCtx({
        sampleRate: this.sampleRate,
        latencyHint: 'interactive',
      });
      // Setup output gain node (untuk volume remote & deafen)
      this.playbackGain = this.audioContext.createGain();
      this.playbackGain.connect(this.audioContext.destination);
    }

    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }
  }

  /**
   * Mulai merekam dari mikrofon
   */
  async startRecording(onChunkCallback) {
    if (this.isRecording) return;
    await this.ensureAudioContext();

    this.onAudioChunk = onChunkCallback;

    try {
      this.mediaStream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: true,
          noiseSuppression: true,
          autoGainControl: true,
          channelCount: 1,
          sampleRate: this.sampleRate,
        },
      });

      this.micSource = this.audioContext.createMediaStreamSource(this.mediaStream);

      // Gunakan ScriptProcessorNode untuk stream PCM berlatensi rendah
      this.processorNode = this.audioContext.createScriptProcessor(
        this.bufferSize,
        1, // 1 input channel (mono)
        1  // 1 output channel
      );

      this.processorNode.onaudioprocess = (event) => {
        if (!this.isRecording) return;

        const inputBuffer = event.inputBuffer.getChannelData(0);

        // 1. Hitung RMS Volume untuk visualizer lokal
        let sumSquares = 0;
        for (let i = 0; i < inputBuffer.length; i++) {
          sumSquares += inputBuffer[i] * inputBuffer[i];
        }
        const rms = Math.sqrt(sumSquares / inputBuffer.length);
        this.localVolume = this.isMuted ? 0 : Math.min(100, Math.round(rms * 180));

        // Jika dimute, jangan kirim data audio
        if (this.isMuted) return;

        // 2. Konversi Float32 (-1.0 .. 1.0) ke Int16 PCM (-32768 .. 32767)
        const int16Array = new Int16Array(inputBuffer.length);
        for (let i = 0; i < inputBuffer.length; i++) {
          const s = Math.max(-1, Math.min(1, inputBuffer[i]));
          int16Array[i] = s < 0 ? s * 0x8000 : s * 0x7fff;
        }

        // 3. Panggil callback pengirim
        if (this.onAudioChunk) {
          this.onAudioChunk(int16Array);
        }
      };

      this.micSource.connect(this.processorNode);
      // Hubungkan processor ke destination dengan gain 0 agar tidak echo ke speaker sendiri
      const zeroGain = this.audioContext.createGain();
      zeroGain.gain.value = 0;
      this.processorNode.connect(zeroGain);
      zeroGain.connect(this.audioContext.destination);

      this.isRecording = true;
      return true;
    } catch (err) {
      console.error('Gagal mengakses mikrofon:', err);
      throw err;
    }
  }

  /**
   * Putar paket audio PCM (Int16Array) yang sudah didekripsi
   */
  playChunk(pcmInt16Array) {
    if (!this.audioContext || this.isDeafened) {
      this.remoteVolume = 0;
      return;
    }

    const length = pcmInt16Array.length;
    const float32 = new Float32Array(length);

    // Konversi Int16 PCM kembali ke Float32
    let sumSquares = 0;
    for (let i = 0; i < length; i++) {
      const val = pcmInt16Array[i] / 0x8000;
      float32[i] = val;
      sumSquares += val * val;
    }

    // Hitung RMS Remote Volume untuk visualizer
    const rms = Math.sqrt(sumSquares / length);
    this.remoteVolume = Math.min(100, Math.round(rms * 180));

    // Buat AudioBuffer dan jadwalkan pemutaran halus
    const audioBuffer = this.audioContext.createBuffer(1, length, this.sampleRate);
    audioBuffer.getChannelData(0).set(float32);

    const source = this.audioContext.createBufferSource();
    source.buffer = audioBuffer;
    source.connect(this.playbackGain);

    const currentTime = this.audioContext.currentTime;
    // Jitter buffer 25ms jika waktu pemutaran tertinggal
    if (this.nextStartTime < currentTime) {
      this.nextStartTime = currentTime + 0.025;
    }

    source.start(this.nextStartTime);
    this.nextStartTime += audioBuffer.duration;
  }

  /**
   * Toggle atau set status Mute mikrofon
   */
  setMute(state) {
    this.isMuted = typeof state === 'boolean' ? state : !this.isMuted;
    if (this.mediaStream) {
      this.mediaStream.getAudioTracks().forEach((track) => {
        track.enabled = !this.isMuted;
      });
    }
    if (this.isMuted) this.localVolume = 0;
    return this.isMuted;
  }

  /**
   * Toggle atau set status Deafen (matikan suara penerima)
   */
  setDeafen(state) {
    this.isDeafened = typeof state === 'boolean' ? state : !this.isDeafened;
    if (this.playbackGain) {
      this.playbackGain.gain.value = this.isDeafened ? 0 : 1;
    }
    if (this.isDeafened) this.remoteVolume = 0;
    return this.isDeafened;
  }

  /**
   * Stop rekaman mikrofon
   */
  stopRecording() {
    this.isRecording = false;
    this.localVolume = 0;

    if (this.processorNode) {
      this.processorNode.disconnect();
      this.processorNode = null;
    }

    if (this.micSource) {
      this.micSource.disconnect();
      this.micSource = null;
    }

    if (this.mediaStream) {
      this.mediaStream.getTracks().forEach((t) => t.stop());
      this.mediaStream = null;
    }
  }

  /**
   * Tutup seluruh audio context
   */
  async close() {
    this.stopRecording();
    this.remoteVolume = 0;
    if (this.audioContext && this.audioContext.state !== 'closed') {
      await this.audioContext.close();
      this.audioContext = null;
    }
  }
}
