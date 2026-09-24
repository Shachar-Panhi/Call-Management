import { useState, useEffect, useRef, useCallback } from 'react';
import { ConnectionPacketSchema, SdpPacketSchema, IcePacketSchema, type LogMessage, type MessageType } from '../types';

export const useWebRTC = (url: string) => {
  const [messages, setMessages] = useState<LogMessage[]>([]);
  const [isDataChannelOpen, setIsDataChannelOpen] = useState<boolean>(false);
  
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const dcRef = useRef<RTCDataChannel | null>(null);
  const messageIdRef = useRef<number>(0);
  const isConnecting = useRef<boolean>(false);
  const sessionIdRef = useRef<string>('');

  const appendLog = useCallback((text: string, type: MessageType) => {
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  }, []);

  const sendMessage = useCallback((input: string) => {
    if (isDataChannelOpen && dcRef.current && dcRef.current.readyState === 'open') {
      dcRef.current.send(input);
      appendLog(`Sent: ${input}`, 'sent');
      return true;
    }
    appendLog('Cannot send message: WebRTC connection not established yet.', 'system');
    return false;
  }, [isDataChannelOpen, appendLog]);

  useEffect(() => {
    if (wsRef.current || isConnecting.current) return;
    
    isConnecting.current = true;
    const ws = new WebSocket(url);
    wsRef.current = ws;

    const pc = new RTCPeerConnection();
    pcRef.current = pc;

    pc.onicecandidate = (event) => {
      if (event.candidate && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
          session_id: sessionIdRef.current,
          candidate: event.candidate.candidate,
          sdpMid: event.candidate.sdpMid
        }));
      }
    };

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
        appendLog(`Non-JSON WebSocket message: ${event.data}`, 'received');
      }
    };

    ws.onclose = () => {
      appendLog('WebSocket disconnected.', 'system');
      wsRef.current = null;
      isConnecting.current = false;
    };

    return () => {
      pc.close();
      ws.close();
    };
  }, [url, appendLog]);

  return { messages, isDataChannelOpen, sendMessage };
};