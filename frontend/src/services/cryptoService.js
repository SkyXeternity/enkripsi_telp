/**
 * CipherVoice Cryptographic Service
 * Mengimplementasikan AES-256-GCM yang 100% kompatibel dengan backend C (OpenSSL).
 *
 * Parameter Kriptografi (Sesuai backend/src/crypto.h):
 * - KEY_SIZE   : 32 byte (256-bit)
 * - NONCE_SIZE : 12 byte (96-bit)
 * - TAG_SIZE   : 16 byte (128-bit)
 *
 * Format Paket (Sesuai backend/urutan.md Tahap 6):
 * [0..3]   : Sequence Number (Uint32 big-endian, 4 byte)
 * [4..15]  : Nonce / IV (12 byte)
 * [16..N]  : Ciphertext (Panjang dinamis)
 * [N..N+15]: Authentication Tag (16 byte, berada di akhir buffer AES-GCM Web Crypto)
 */

export const KEY_SIZE = 32;
export const NONCE_SIZE = 12;
export const TAG_SIZE = 16;
export const HEADER_SIZE = 4 + NONCE_SIZE; // 16 byte

/**
 * Konversi Uint8Array ke Hex String
 */
export function bytesToHex(bytes) {
  return Array.from(bytes)
    .map(b => b.toString(16).padStart(2, '0'))
    .join('');
}

/**
 * Konversi Hex String ke Uint8Array
 */
export function hexToBytes(hex) {
  const cleanHex = hex.replace(/[^0-9a-fA-F]/g, '');
  const bytes = new Uint8Array(cleanHex.length / 2);
  for (let i = 0; i < cleanHex.length; i += 2) {
    bytes[i / 2] = parseInt(cleanHex.substring(i, i + 2), 16);
  }
  return bytes;
}

/**
 * Generate Kunci Acak 256-bit AES-GCM
 */
export async function generateRandomKey() {
  const raw = new Uint8Array(KEY_SIZE);
  window.crypto.getRandomValues(raw);
  const key = await window.crypto.subtle.importKey(
    'raw',
    raw,
    { name: 'AES-GCM' },
    true,
    ['encrypt', 'decrypt']
  );
  return {
    key,
    raw,
    hex: bytesToHex(raw),
  };
}

/**
 * Import Kunci dari Hex String (64 karakter hex)
 */
export async function importKeyFromHex(hex) {
  const raw = hexToBytes(hex);
  if (raw.length !== KEY_SIZE) {
    throw new Error(`Kunci harus berukuran ${KEY_SIZE} byte (64 hex characters)`);
  }
  const key = await window.crypto.subtle.importKey(
    'raw',
    raw,
    { name: 'AES-GCM' },
    true,
    ['encrypt', 'decrypt']
  );
  return { key, raw, hex: bytesToHex(raw) };
}

/**
 * Derivasi Kunci AES-256 dari Passphrase menggunakan PBKDF2 + SHA-256
 */
export async function deriveKeyFromPassphrase(passphrase, salt = 'CipherVoice-Salt-2026') {
  const enc = new TextEncoder();
  const keyMaterial = await window.crypto.subtle.importKey(
    'raw',
    enc.encode(passphrase),
    'PBKDF2',
    false,
    ['deriveKey', 'deriveBits']
  );

  const saltBytes = typeof salt === 'string' ? enc.encode(salt) : salt;

  const key = await window.crypto.subtle.deriveKey(
    {
      name: 'PBKDF2',
      salt: saltBytes,
      iterations: 100000,
      hash: 'SHA-256',
    },
    keyMaterial,
    { name: 'AES-GCM', length: 256 },
    true,
    ['encrypt', 'decrypt']
  );

  const raw = new Uint8Array(await window.crypto.subtle.exportKey('raw', key));
  return { key, raw, hex: bytesToHex(raw) };
}

/**
 * Enkripsi payload dan bentuk format paket:
 * [seq (4B)] + [nonce (12B)] + [ciphertext (N bytes)] + [tag (16B)]
 */
export async function encryptPacket(plaintextBuffer, cryptoKey, seq = 0) {
  const nonce = new Uint8Array(NONCE_SIZE);
  window.crypto.getRandomValues(nonce);

  const encryptedWithTag = await window.crypto.subtle.encrypt(
    {
      name: 'AES-GCM',
      iv: nonce,
      tagLength: TAG_SIZE * 8, // 128 bit
    },
    cryptoKey,
    plaintextBuffer
  );

  const encryptedBytes = new Uint8Array(encryptedWithTag);
  const totalLength = 4 + NONCE_SIZE + encryptedBytes.byteLength;
  const packet = new Uint8Array(totalLength);
  const view = new DataView(packet.buffer);

  // 1. Sequence number (4 byte big-endian)
  view.setUint32(0, seq, false);

  // 2. Nonce / IV (12 byte)
  packet.set(nonce, 4);

  // 3. Ciphertext + Tag (N + 16 byte)
  packet.set(encryptedBytes, 16);

  return packet;
}

/**
 * Dekripsi paket [seq][nonce][ciphertext][tag]
 */
export async function decryptPacket(packetBuffer, cryptoKey) {
  const bytes = packetBuffer instanceof Uint8Array ? packetBuffer : new Uint8Array(packetBuffer);

  // Minimum length: 4 byte seq + 12 byte nonce + 0 byte payload + 16 byte tag = 32 byte
  if (bytes.byteLength < 4 + NONCE_SIZE + TAG_SIZE) {
    return {
      isValid: false,
      error: 'Ukuran paket terlalu pendek (di bawah 32 byte)',
      seq: 0,
    };
  }

  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const seq = view.getUint32(0, false);
  const nonce = bytes.slice(4, 16);
  const cipherAndTag = bytes.slice(16);

  try {
    const decryptedBuffer = await window.crypto.subtle.decrypt(
      {
        name: 'AES-GCM',
        iv: nonce,
        tagLength: TAG_SIZE * 8,
      },
      cryptoKey,
      cipherAndTag
    );

    const ciphertextLen = cipherAndTag.byteLength - TAG_SIZE;
    const tag = cipherAndTag.slice(ciphertextLen);

    return {
      isValid: true,
      seq,
      nonce,
      tag,
      plaintext: decryptedBuffer,
      ciphertextLength: ciphertextLen,
    };
  } catch (err) {
    return {
      isValid: false,
      seq,
      nonce,
      error: 'Tag autentikasi tidak valid atau data rusak (GCM Auth Failed)',
      originalError: err,
    };
  }
}

/**
 * Bedah struktur paket untuk Packet Inspector UI
 */
export function inspectPacket(packetBuffer) {
  const bytes = packetBuffer instanceof Uint8Array ? packetBuffer : new Uint8Array(packetBuffer);
  if (bytes.byteLength < 32) {
    return { validFormat: false, totalBytes: bytes.byteLength };
  }

  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const seq = view.getUint32(0, false);
  const nonce = bytes.slice(4, 16);
  const cipherAndTag = bytes.slice(16);
  const cipherLen = Math.max(0, cipherAndTag.byteLength - TAG_SIZE);
  const ciphertext = cipherAndTag.slice(0, cipherLen);
  const tag = cipherAndTag.slice(cipherLen);

  return {
    validFormat: true,
    seq,
    totalBytes: bytes.byteLength,
    nonceHex: bytesToHex(nonce),
    ciphertextHex: bytesToHex(ciphertext),
    tagHex: bytesToHex(tag),
    ciphertextLength: cipherLen,
  };
}

/**
 * Simulasikan kerusakan data (Tamper Simulation) untuk tes ketahanan (Tahap 7)
 */
export function tamperPacket(packetBytes, target = 'ciphertext') {
  const tampered = new Uint8Array(packetBytes);
  if (tampered.length < 32) return tampered;

  if (target === 'tag') {
    // Ubah 1 byte di bagian tag (16 byte terakhir)
    const tagIndex = tampered.length - 1;
    tampered[tagIndex] ^= 0xff;
  } else if (target === 'nonce') {
    // Ubah 1 byte di nonce (offset 4..15)
    tampered[4] ^= 0xaa;
  } else {
    // Ubah 1 byte di ciphertext (offset 16)
    tampered[16] ^= 0x55;
  }

  return tampered;
}
