import React, { useState, useEffect, useRef } from 'react';

type MessageType = 'sent' | 'received' | 'system';

interface LogMessage {
  id: number;
  text: string;
  type: MessageType;
}

export default function App() {
  const [messages, setMessages] = useState<LogMessage[]>([]);
  const [input, setInput] = useState<string>('');
  const [isDataChannelOpen, setIsDataChannelOpen] = useState<boolean>(false);
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const dcRef = useRef<RTCDataChannel | null>(null);
  const logEndRef = useRef<HTMLDivElement>(null);
  const messageIdRef = useRef<number>(0);

  const appendLog = (text: string, type: MessageType) => {
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  };

  useEffect(() => {
    // 1. Connect to the Boost.Beast WebSocket
    const ws = new WebSocket('ws://127.0.0.1:8080');
    wsRef.current = ws;

    // 2. Initialize WebRTC with the same STUN server as the C++ backend
    const pc = new RTCPeerConnection({
      iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
    });
    pcRef.current = pc;

    // 3. Send our ICE candidates to C++ formatted exactly like the SignalingPacket struct
    pc.onicecandidate = (event) => {
      if (event.candidate && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
          type: 'candidate',
          candidate: event.candidate.candidate,
          sdpMid: event.candidate.sdpMid
        }));
      }
    };

    // 4. Listen for the DataChannel created by the C++ backend
    pc.ondatachannel = (event) => {
      const dc = event.channel;
      dcRef.current = dc;

      dc.onopen = () => {
        setIsDataChannelOpen(true);
        appendLog(`WebRTC DataChannel '${dc.label}' opened!`, 'system');
      };

      dc.onmessage = (e) => {
        appendLog(e.data, 'received');
      };

      dc.onclose = () => {
        setIsDataChannelOpen(false);
        appendLog('WebRTC DataChannel closed.', 'system');
        dcRef.current = null;
      };
    };

    ws.onopen = () => {
      appendLog('WebSocket connected. Type "start" to initiate WebRTC handshake.', 'system');
    };

    // 5. Parse the JSON sent by Glaze
    ws.onmessage = async (event) => {
      if (event.data === 'start') return; // Ignore our own start command echo if it happens

      try {
        const packet = JSON.parse(event.data);

        if (packet.type === 'offer' && packet.sdp) {
          appendLog('Received WebRTC offer, generating answer...', 'system');
          await pc.setRemoteDescription(new RTCSessionDescription({ type: 'offer', sdp: packet.sdp }));
          
          const answer = await pc.createAnswer();
          await pc.setLocalDescription(answer);
          
          // Send the answer formatted for the Glaze parser
          ws.send(JSON.stringify({ type: 'answer', sdp: answer.sdp }));
        } 
        else if (packet.type === 'candidate' && packet.candidate && packet.sdpMid) {
          await pc.addIceCandidate(new RTCIceCandidate({
            candidate: packet.candidate,
            sdpMid: packet.sdpMid
          }));
        }
      } catch (err) {
        // Only log if it's not a JSON packet (shouldn't happen with the new backend)
        appendLog(`Non-JSON WebSocket message: ${event.data}`, 'received');
      }
    };

    ws.onclose = () => {
      appendLog('WebSocket disconnected.', 'system');
    };

    return () => {
      pc.close();
      ws.close();
    };
  }, []);

  useEffect(() => {
    if (logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages]);

  const sendMessage = () => {
    if (input) {
      if (input === 'start' && wsRef.current?.readyState === WebSocket.OPEN) {
        wsRef.current.send(input);
        appendLog('Sent: start (via WebSocket)', 'sent');
      } else if (isDataChannelOpen && dcRef.current && dcRef.current.readyState === 'open') {
        dcRef.current.send(input);
        appendLog(`Sent: ${input} (via WebRTC)`, 'sent');
      } else {
        appendLog('Cannot send message: WebRTC connection not open yet.', 'system');
      }
      setInput('');
    }
  };

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      sendMessage();
    }
  };

  const getMessageStyle = (type: MessageType): React.CSSProperties => {
    switch (type) {
      case 'sent':
        return { color: 'blue', margin: '5px 0' };
      case 'received':
        return { color: 'green', margin: '5px 0' };
      case 'system':
        return { color: 'gray', fontStyle: 'italic', margin: '5px 0' };
      default:
        return {};
    }
  };

  return (
    <div style={{ fontFamily: 'sans-serif', padding: '20px', maxWidth: '600px', margin: '0 auto' }}>
      <h2>Signaling & WebRTC Console</h2>
      <div
        style={{
          border: '1px solid #ccc',
          height: '400px',
          overflowY: 'scroll',
          padding: '10px',
          marginBottom: '10px',
          textAlign: 'left',
          backgroundColor: '#fafafa'
        }}
      >
        {messages.map((msg) => (
          <div key={msg.id} style={getMessageStyle(msg.type)}>
            {msg.text}
          </div>
        ))}
        <div ref={logEndRef} />
      </div>
      <div style={{ display: 'flex' }}>
        <input
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={handleKeyDown}
          placeholder={isDataChannelOpen ? "Type message to send over WebRTC..." : "Type 'start' to begin..."}
          autoFocus
          style={{ flexGrow: 1, marginRight: '10px', padding: '10px' }}
        />
        <button onClick={sendMessage} style={{ padding: '10px 20px', cursor: 'pointer' }}>
          Send
        </button>
      </div>
    </div>
  );
}