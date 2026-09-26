import { useState, useRef, useCallback } from 'react';
import { ConnectionPacketSchema, SdpPacketSchema, IcePacketSchema, type LogMessage, type MessageType } from '../types';

export const useWebRTC = (url: string) => {
  const [messages, setMessages] = useState<LogMessage[]>([]);
  const [isConnected, setIsConnected] = useState<boolean>(false);
  
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const messageIdRef = useRef<number>(0);
  const sessionIdRef = useRef<string>('');

  const appendLog = useCallback((text: string, type: MessageType) => {
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  }, []);

  const startCall = async () => {
    if (wsRef.current) return;

    try {
      appendLog('Requesting microphone access...', 'system');
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      streamRef.current = stream;
      appendLog('Microphone access granted.', 'system');

      const ws = new WebSocket(url);
      wsRef.current = ws;

      const pc = new RTCPeerConnection();
      pcRef.current = pc;

      stream.getTracks().forEach((track) => {
        pc.addTrack(track, stream);
      });

      pc.onicecandidate = (event) => {
        if (event.candidate && ws.readyState === WebSocket.OPEN) {
          ws.send(JSON.stringify({
            session_id: sessionIdRef.current,
            candidate: event.candidate.candidate,
            sdpMid: event.candidate.sdpMid
          }));
        }
      };

      pc.ontrack = () => {
        appendLog('Received remote media track from server.', 'system');
      };

      ws.onopen = () => {
        setIsConnected(true);
        appendLog('WebSocket connected. Waiting for server to initiate WebRTC handshake...', 'system');
      };

      ws.onmessage = async (event) => {
          try {
            const rawPacket = JSON.parse(event.data);

            if (rawPacket.sdp) {
              const parsed = SdpPacketSchema.safeParse(rawPacket);
              if (parsed.success) {
                appendLog('Received WebRTC offer, generating answer...', 'system');
                
                await pc.setRemoteDescription(new RTCSessionDescription({ type: 'offer', sdp: parsed.data.sdp }));
                
                const answer = await pc.createAnswer();
                await pc.setLocalDescription(answer);
                
                ws.send(JSON.stringify({ 
                  session_id: sessionIdRef.current,
                  sdp: answer.sdp 
                }));
              }
            } 
            else if (rawPacket.candidate) {
              const parsed = IcePacketSchema.safeParse(rawPacket);
              if (parsed.success) {
                await pc.addIceCandidate(new RTCIceCandidate({
                  candidate: parsed.data.candidate,
                  sdpMid: parsed.data.sdpMid ?? null
                }));
              }
            }
            else if (rawPacket.session_id) {
              const parsed = ConnectionPacketSchema.safeParse(rawPacket);
              if (parsed.success) {
                sessionIdRef.current = parsed.data.session_id;
                appendLog(`Received session ID: ${parsed.data.session_id}`, 'system');
              }
            } else {
              appendLog(`Ignored invalid signaling message: ${event.data}`, 'received');
            }
          } catch (err) {
            appendLog(`Error processing message: ${err}`, 'system');
            console.error("WebSocket message error:", err);
          }
        };

      ws.onclose = () => {
        setIsConnected(false);
        appendLog('WebSocket disconnected.', 'system');
        wsRef.current = null;
      };

    } catch (err) {
      appendLog(`Failed to access microphone: ${err}`, 'system');
    }
  };

  const stopCall = () => {
    if (streamRef.current) {
      streamRef.current.getTracks().forEach(track => track.stop());
      streamRef.current = null;
    }
    if (pcRef.current) {
      pcRef.current.close();
      pcRef.current = null;
    }
    if (wsRef.current) {
      wsRef.current.close();
      wsRef.current = null;
    }
    setIsConnected(false);
    appendLog('Call stopped.', 'system');
  };

  return { messages, isConnected, startCall, stopCall };
};