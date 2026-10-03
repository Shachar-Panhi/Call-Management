import { useState, useRef, useCallback } from 'react';
import { 
  ConnectionPacketSchema, 
  SdpPacketSchema, 
  IcePacketSchema, 
  ApiResponsePacketSchema,
  type LogMessage, 
  type MessageType 
} from '../types';

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

      const stream = await navigator.mediaDevices.getUserMedia({ 
        audio: { 
          sampleRate: 16000,
          channelCount: 1,
          echoCancellation: true,
          noiseSuppression: true
        } 
      });      
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
        appendLog('Received remote media track from partner.', 'system');
      };

      ws.onopen = () => {
        setIsConnected(true);
        appendLog('WebSocket connected. Joining matching queue...', 'system');
        ws.send(JSON.stringify({ action: 'join_queue' }));
      };

      ws.onmessage = async (event) => {
        try {
          const rawPacket = JSON.parse(event.data);

          const apiParsed = ApiResponsePacketSchema.safeParse(rawPacket);
          if (apiParsed.success && apiParsed.data.type === 'api_response') {
            const { action, status } = apiParsed.data;
            
            if (action === 'join_queue' && status === 'queued') {
              appendLog(`Queued on server. Waiting for match...`, 'system');
            } else if (action === 'match' && status === 'matched') {
              appendLog(`Partner matched! Waiting for WebRTC initialization...`, 'system');
            } else if (action === 'disconnect' && status === 'disconnected') {
              appendLog(`Call disconnected by server.`, 'system');
              if (pcRef.current) {
                pcRef.current.close();
              }
            }
            return;
          }

          const sdpParsed = SdpPacketSchema.safeParse(rawPacket);
          if (sdpParsed.success) {
            appendLog('Received WebRTC offer, generating answer...', 'system');
            
            await pc.setRemoteDescription(new RTCSessionDescription({ type: 'offer', sdp: sdpParsed.data.sdp }));
            
            const answer = await pc.createAnswer();
            await pc.setLocalDescription(answer);
            
            ws.send(JSON.stringify({ 
              session_id: sessionIdRef.current,
              sdp: answer.sdp 
            }));
            return;
          }
          
          const iceParsed = IcePacketSchema.safeParse(rawPacket);
          if (iceParsed.success) {
            await pc.addIceCandidate(new RTCIceCandidate({
              candidate: iceParsed.data.candidate,
              sdpMid: iceParsed.data.sdpMid ?? null
            }));
            return;
          }
          
          const connParsed = ConnectionPacketSchema.safeParse(rawPacket);
          if (connParsed.success) {
            sessionIdRef.current = connParsed.data.session_id;
            appendLog(`Received session ID: ${connParsed.data.session_id}`, 'system');
            return;
          }

          appendLog(`Ignored unhandled message: ${event.data}`, 'received');

        } catch (err) {
          appendLog(`Error processing message: ${err}`, 'system');
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
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ action: 'disconnect' }));
    }

    if (streamRef.current) {
      streamRef.current.getTracks().forEach(track => track.stop());
      streamRef.current = null;
    }
    
    if (pcRef.current) {
      pcRef.current.close();
      pcRef.current = null;
    }
    
    setIsConnected(false);
    appendLog('Microphone access released and call stopped locally.', 'system');
  };

  return { messages, isConnected, startCall, stopCall };
};