import { useState, useRef, useCallback, useEffect } from 'react';
import axios from 'axios';
import { 
  ConnectionPacketSchema, 
  SdpPacketSchema, 
  IcePacketSchema, 
  ApiResponsePacketSchema,
  type LogMessage, 
  type MessageType 
} from '../types';

export const useWebRTC = (wsUrl: string, apiUrl: string) => {
  const [messages, setMessages] = useState<LogMessage[]>([]);
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [sessionId, setSessionId] = useState<string>('');
  const [isInCall, setIsInCall] = useState<boolean>(false);
  
  const isMountedRef = useRef<boolean>(true);
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const micPromiseRef = useRef<Promise<void> | null>(null);
  const messageIdRef = useRef<number>(0);
  const sessionIdRef = useRef<string>('');
  const iceCandidateQueueRef = useRef<RTCIceCandidateInit[]>([]);
  const remoteAudioRef = useRef<HTMLAudioElement | null>(null);

  const appendLog = useCallback((text: string, type: MessageType) => {
    if (!isMountedRef.current) return;
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  }, []);

  const cleanupCall = useCallback(() => {
    if (streamRef.current) {
      streamRef.current.getTracks().forEach((track) => track.stop());
      streamRef.current = null;
    }
    if (pcRef.current) {
      pcRef.current.close();
      pcRef.current = null;
    }
    if (remoteAudioRef.current) {
      remoteAudioRef.current.srcObject = null;
    }
    micPromiseRef.current = null;
    iceCandidateQueueRef.current = [];
    if (isMountedRef.current) {
      setIsInCall(false);
    }
  }, []);

  useEffect(() => {
    isMountedRef.current = true;
    remoteAudioRef.current = new Audio();
    remoteAudioRef.current.autoplay = true;

    return () => {
      isMountedRef.current = false;
      if (remoteAudioRef.current) {
        remoteAudioRef.current.srcObject = null;
        remoteAudioRef.current = null;
      }
      if (wsRef.current) {
        wsRef.current.onopen = null;
        wsRef.current.onmessage = null;
        wsRef.current.onerror = null;
        wsRef.current.onclose = null;
        wsRef.current.close();
        wsRef.current = null;
      }
      cleanupCall();
    };
  }, [cleanupCall]);

  const initCall = useCallback(async () => {
    if (pcRef.current) {
      if (micPromiseRef.current) {
        await micPromiseRef.current;
      }
      return;
    }

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

    pc.ontrack = (event) => {
      appendLog('Received remote media track from partner.', 'system');
      if (remoteAudioRef.current) {
        if (event.streams && event.streams.length > 0) {
          remoteAudioRef.current.srcObject = event.streams[0];
        } else {
          remoteAudioRef.current.srcObject = new MediaStream([event.track]);
        }
      }
    };

    appendLog('Requesting microphone access...', 'system');
    const micPromise = navigator.mediaDevices.getUserMedia({ 
      audio: { sampleRate: 16000, channelCount: 1, echoCancellation: true, noiseSuppression: true } 
    }).then(stream => {
      if (!pcRef.current) {
        stream.getTracks().forEach((track) => track.stop());
        return;
      }
      streamRef.current = stream;
      stream.getTracks().forEach((track) => pc.addTrack(track, stream));
      appendLog('Microphone access granted.', 'system');
    }).catch(err => {
      appendLog(`Failed to access microphone: ${err}`, 'system');
    });

    micPromiseRef.current = micPromise;
    await micPromise;
  }, [appendLog]);

  const connectToServer = useCallback(() => {
    if (wsRef.current) {
      if (
        wsRef.current.readyState === WebSocket.CONNECTING ||
        wsRef.current.readyState === WebSocket.OPEN
      ) {
        return;
      }
      wsRef.current.onopen = null;
      wsRef.current.onmessage = null;
      wsRef.current.onerror = null;
      wsRef.current.onclose = null;
      wsRef.current.close();
      wsRef.current = null;
    }

    const ws = new WebSocket(wsUrl);
    wsRef.current = ws;

    ws.onopen = () => {
      setIsConnected(true);
      appendLog('WebSocket connected. Waiting for connection target.', 'system');
    };

    ws.onerror = () => {
      appendLog('WebSocket connection error.', 'system');
    };

    ws.onmessage = async (event) => {
      try {
        const rawPacket = JSON.parse(event.data);

        const apiParsed = ApiResponsePacketSchema.safeParse(rawPacket);
        if (apiParsed.success && apiParsed.data.type === 'api_response') {
          const { action, status } = apiParsed.data;
          
          if (action === 'match') {
            setIsInCall(true);
            appendLog(`Partner matched! Status: ${status}. Waiting for WebRTC initialization...`, 'system');
            await initCall();
          } else if (action === 'disconnect') {
            cleanupCall();
            appendLog(`Call disconnected by server. Status: ${status}.`, 'system');
          }
          return;
        }

        const sdpParsed = SdpPacketSchema.safeParse(rawPacket);
        if (sdpParsed.success) {
          appendLog('Received WebRTC offer, generating answer...', 'system');
          
          await initCall();

          const pc = pcRef.current;
          if (!pc) {
            appendLog('PeerConnection not available to handle offer.', 'system');
            return;
          }
          
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
  }, [wsUrl, appendLog, initCall, cleanupCall]);

  const connectToPeer = useCallback(async (targetId: string) => {
    if (!isInCall && sessionIdRef.current) {
      appendLog(`Dialing session ID: ${targetId}...`, 'sent');
      try {
        const response = await axios.post(`${apiUrl}/api/call`, {
          session_id: sessionIdRef.current,
          target_session_id: targetId
        });
        
        if (response.data.status !== 'success') {
          appendLog(`Call failed: ${response.data.status || 'unknown error'}`, 'system');
          cleanupCall();
        }
      } catch (err: unknown) {
        if (axios.isAxiosError(err)) {
          appendLog(`Call request failed: ${err.response?.data?.status || err.message}`, 'system');
        } else {
          appendLog(`Call request failed: ${err}`, 'system');
        }
        cleanupCall();
      }
    }
  }, [isInCall, apiUrl, appendLog, cleanupCall]);

  const stopCall = useCallback(async () => {
    if (sessionIdRef.current) {
      try {
        await axios.post(`${apiUrl}/api/end_call`, { 
          session_id: sessionIdRef.current 
        });
      } catch (err: unknown) {
        if (axios.isAxiosError(err)) {
          appendLog(`Failed to send end call request: ${err.response?.data?.status || err.message}`, 'system');
        } else {
          appendLog(`Failed to send end call request: ${err}`, 'system');
        }
      }
    }
    cleanupCall();
    appendLog('Microphone access released and call ended.', 'system');
  }, [apiUrl, appendLog, cleanupCall]);

  return { messages, isConnected, sessionId, isInCall, connectToServer, stopCall, connectToPeer };
};