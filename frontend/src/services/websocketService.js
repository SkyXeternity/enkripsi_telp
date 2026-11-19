/**
 * CipherVoice WebSocket & Signaling Service
 * Menghubungkan client dengan server C backend (Mongoose WebSocket).
 *
 * Protokol Signaling:
 * - Client -> Server:
 *   - { "type": "create_room" }
 *   - { "type": "join_room", "code": "000001" }
 *   - { "type": "leave_room" }
 * - Server -> Client:
 *   - { "type": "created", "code": "000001" }
 *   - { "type": "joined", "code": "000001" }
 *   - { "type": "peer_joined", "code": "000001" }
 *   - { "type": "peer_left" }
 *   - { "type": "error", "message": "..." }
 * - Relay Biner (Voice Packet):
 *   - Mongoose meneruskan byte mentah: [seq][nonce][ciphertext][tag]
 */

export class WebSocketService {
  constructor() {
    this.ws = null;
    this.url = 'wss://outskirts-equator-capricorn.ngrok-free.dev/websocket';
    this.status = 'DISCONNECTED'; // DISCONNECTED, CONNECTING, CONNECTED, IN_ROOM
    this.currentRoom = null;
    this.isPeerConnected = false;
    this.isLoopbackMode = false; // Mode simulasi offline / testing lokal

    // Event callbacks
    this.onStatusChange = null;
    this.onRoomCreated = null;
    this.onRoomJoined = null;
    this.onPeerJoined = null;
    this.onPeerLeft = null;
    this.onError = null;
    this.onBinaryReceived = null;
    this.onChatMessage = null;
  }

  setStatus(status) {
    this.status = status;
    if (this.onStatusChange) {
      this.onStatusChange(status);
    }
  }

  /**
   * Koneksi ke server WebSocket backend C
   */
  connect(customUrl) {
    if (this.isLoopbackMode) {
      this.setStatus('CONNECTED');
      return;
    }

    if (customUrl) this.url = customUrl;

    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      return;
    }

    this.setStatus('CONNECTING');

    try {
      this.ws = new WebSocket(this.url);
      this.ws.binaryType = 'arraybuffer';

      this.ws.onopen = () => {
        this.setStatus('CONNECTED');
      };

      this.ws.onmessage = (event) => {
        // Jika data biner (paket audio terenkripsi relay)
        if (event.data instanceof ArrayBuffer) {
          if (this.onBinaryReceived) {
            this.onBinaryReceived(event.data);
          }
          return;
        }

        // Jika data teks (signaling JSON)
        try {
          const msg = JSON.parse(event.data);
          this.handleSignalingMessage(msg);
        } catch (e) {
          // Jika backend mengirim teks biasa (misal echo testing)
          console.log('[WS Text Raw]:', event.data);
        }
      };

      this.onerror = (err) => {
        console.error('[WS Error]:', err);
        if (this.onError) this.onError(err);
      };

      this.ws.onclose = () => {
        this.setStatus('DISCONNECTED');
        this.currentRoom = null;
        this.isPeerConnected = false;
      };
    } catch (err) {
      this.setStatus('DISCONNECTED');
      if (this.onError) this.onError(err);
    }
  }

  /**
   * Handle pesan JSON dari backend
   */
  handleSignalingMessage(msg) {
    switch (msg.type) {
      case 'created':
        this.currentRoom = msg.code;
        this.setStatus('IN_ROOM');
        if (this.onRoomCreated) this.onRoomCreated(msg.code);
        break;

      case 'joined':
        this.currentRoom = msg.code;
        this.isPeerConnected = true;
        this.setStatus('IN_ROOM');
        if (this.onRoomJoined) this.onRoomJoined(msg.code);
        break;

      case 'peer_joined':
        this.isPeerConnected = true;
        if (this.onPeerJoined) this.onPeerJoined(msg);
        break;

      case 'peer_left':
        this.isPeerConnected = false;
        if (this.onPeerLeft) this.onPeerLeft(msg);
        break;

      case 'chat':
        if (this.onChatMessage) this.onChatMessage(msg);
        break;

      case 'error':
        if (this.onError) this.onError(new Error(msg.message || 'Server error'));
        break;

      default:
        console.log('[WS Message]:', msg);
    }
  }

  /**
   * Buat room baru di server
   */
  createRoom() {
    if (this.isLoopbackMode) {
      const mockCode = String(Math.floor(100000 + Math.random() * 900000));
      this.currentRoom = mockCode;
      this.isPeerConnected = true; // Di loopback, peer adalah diri sendiri
      this.setStatus('IN_ROOM');
      if (this.onRoomCreated) this.onRoomCreated(mockCode);
      return;
    }

    this.sendJson({ type: 'create_room' });
  }

  /**
   * Bergabung ke room yang sudah ada
   */
  joinRoom(code) {
    if (this.isLoopbackMode) {
      this.currentRoom = code;
      this.isPeerConnected = true;
      this.setStatus('IN_ROOM');
      if (this.onRoomJoined) this.onRoomJoined(code);
      return;
    }

    this.sendJson({ type: 'join_room', code: String(code) });
  }

  /**
   * Keluar dari room
   */
  leaveRoom() {
    if (this.isLoopbackMode) {
      this.currentRoom = null;
      this.isPeerConnected = false;
      this.setStatus('CONNECTED');
      return;
    }

    if (this.currentRoom) {
      // Beritahu server bahwa peer keluar dari room
      this.sendJson({ 
        type: 'peer_left', 
        code: String(this.currentRoom)
      });
    }

    this.currentRoom = null;
    this.isPeerConnected = false;
    this.setStatus('CONNECTED');
  }

  removeRoom() {
    this.leaveRoom();
  }

  /**
   * Kirim pesan JSON teks
   */
  sendJson(obj) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(obj));
    }
  }

  /**
   * Kirim paket biner (Voice Packet terenkripsi)
   */
  sendBinary(bufferOrUint8) {
    const buffer = bufferOrUint8 instanceof Uint8Array ? bufferOrUint8.buffer : bufferOrUint8;

    if (this.isLoopbackMode) {
      // Di loopback mode, teruskan kembali langsung ke listener penerima secara asinkron
      setTimeout(() => {
        if (this.onBinaryReceived) {
          this.onBinaryReceived(buffer.slice(0));
        }
      }, 10);
      return true;
    }

    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(buffer);
      return true;
    }
    return false;
  }

  /**
   * Kirim pesan chat terenkripsi
   */
  sendChat(chatPacket) {
    if (this.isLoopbackMode) {
      setTimeout(() => {
        if (this.onChatMessage) this.onChatMessage(chatPacket);
      }, 20);
      return;
    }
    this.sendJson({ type: 'chat', ...chatPacket });
  }

  /**
   * Putus koneksi WebSocket
   */
  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.setStatus('DISCONNECTED');
    this.currentRoom = null;
    this.isPeerConnected = false;
  }
}
