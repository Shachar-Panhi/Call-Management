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
  const [sessionId, setSessionId] = useState<string>('');
  const [isInCall, setIsInCall] = useState<boolean>(false);
  
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const micPromiseRef = useRef<Promise<void> | null>(null);
  const messageIdRef = useRef<number>(0);
  const sessionIdRef = useRef<string>('');
  const iceCandidateQueueRef = useRef<RTCIceCandidateInit[]>([]);

  const appendLog = useCallback((text: string, type: MessageType) => {
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  }, []);

  const connectToServer = () => {
    if (wsRef.current) return;

    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => {
      setIsConnected(true);
      appendLog('WebSocket connected. Waiting for connection target.', 'system');
    };

    ws.onmessage = async (event) => {
      try {
        const rawPacket = JSON.parse(event.data);

        const apiParsed = ApiResponsePacketSchema.safeParse(rawPacket);
        if (apiParsed.success && apiParsed.data.type === 'api_response') {
          const { action, status } = apiParsed.data;
          
          if (action === 'connect_to') {
            if (status === 'failed') {
              appendLog('Call failed. Partner not found or busy.', 'system');
              cleanupCall();
            } else if (status === 'cannot_call_self') {
              appendLog('Call failed. You cannot call yourself.', 'system');
              cleanupCall();
            }
          } else if (action === 'match') {
            setIsInCall(true);
            initCall();
            appendLog(`Partner matched! Status: ${status}. Waiting for WebRTC initialization...`, 'system');
          } else if (action === 'disconnect') {
            cleanupCall();
            appendLog(`Call disconnected by server. Status: ${status}.`, 'system');
          }
          return;
        }

        const sdpParsed = SdpPacketSchema.safeParse(rawPacket);
        if (sdpParsed.success) {
          appendLog('Received WebRTC offer, generating answer...', 'system');
          
          if (!pcRef.current) {
            initCall();
          }

          if (micPromiseRef.current) {
            await micPromiseRef.current;
          }
          
          const pc = pcRef.current!;
          await pc.setRemoteDescription(new RTCSessionDescription({ type: 'offer', sdp: sdpParsed.data.sdp }));
          
          while (iceCandidateQueueRef.current.length > 0) {
            const candidate = iceCandidateQueueRef.current.shift();
            if (candidate) {
              await pc.addIceCandidate(new RTCIceCandidate(candidate));
            }
          }

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
          const candidateData = {
            candidate: iceParsed.data.candidate,
            sdpMid: iceParsed.data.sdpMid ?? null
          };

          if (pcRef.current?.remoteDescription) {
            await pcRef.current.addIceCandidate(new RTCIceCandidate(candidateData));
          } else {
            iceCandidateQueueRef.current.push(candidateData);
          }
          return;
        }
        
        const connParsed = ConnectionPacketSchema.safeParse(rawPacket);
        if (connParsed.success) {
          sessionIdRef.current = connParsed.data.session_id;
          setSessionId(connParsed.data.session_id);
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
      setIsInCall(false);
      setSessionId('');
      cleanupCall();
      appendLog('WebSocket disconnected.', 'system');
      wsRef.current = null;
    };
  };

  const initCall = () => {
    if (pcRef.current) return;

    const pc = new RTCPeerConnection();
    pcRef.current = pc;
    iceCandidateQueueRef.current = [];

    pc.onicecandidate = (event) => {
      if (event.candidate && wsRef.current?.readyState === WebSocket.OPEN) {
        wsRef.current.send(JSON.stringify({
          session_id: sessionIdRef.current,
          candidate: event.candidate.candidate,
          sdpMid: event.candidate.sdpMid
        }));
      }
    };

    pc.ontrack = () => {
      appendLog('Received remote media track from partner.', 'system');
    };

    appendLog('Requesting microphone access...', 'system');
    micPromiseRef.current = navigator.mediaDevices.getUserMedia({ 
      audio: { sampleRate: 16000, channelCount: 1, echoCancellation: true, noiseSuppression: true } 
    }).then(stream => {
      streamRef.current = stream;
      stream.getTracks().forEach((track) => pc.addTrack(track, stream));
      appendLog('Microphone access granted.', 'system');
    }).catch(err => {
      appendLog(`Failed to access microphone: ${err}`, 'system');
    });

    return micPromiseRef.current;
  };

  const cleanupCall = useCallback(() => {
    if (streamRef.current) {
      streamRef.current.getTracks().forEach(track => track.stop());
      streamRef.current = null;
    }
    if (pcRef.current) {
      pcRef.current.close();
      pcRef.current = null;
    }
    micPromiseRef.current = null;
    iceCandidateQueueRef.current = [];
    setIsInCall(false);
  }, []);

  const connectToPeer = async (targetId: string) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN && !isInCall) {
      wsRef.current.send(JSON.stringify({ 
        action: 'connect_to',
        target_session_id: targetId
      }));
      appendLog(`Dialing session ID: ${targetId}...`, 'sent');
    }
  };

  const stopCall = () => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ action: 'disconnect' }));
    }
    cleanupCall();
    appendLog('Microphone access released and call ended.', 'system');
  };

  return { messages, isConnected, sessionId, isInCall, connectToServer, stopCall, connectToPeer };
};