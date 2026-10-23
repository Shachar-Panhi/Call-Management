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
  const [isPttActive, setIsPttActive] = useState<boolean>(false);
  
  const isMountedRef = useRef<boolean>(true);
  const wsRef = useRef<WebSocket | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const micPromiseRef = useRef<Promise<void> | null>(null);
  const messageIdRef = useRef<number>(0);
  const sessionIdRef = useRef<string>('');
  const iceCandidateQueueRef = useRef<RTCIceCandidateInit[]>([]);
  const remoteAudioRef = useRef<HTMLAudioElement | null>(null);
  const pttActiveRef = useRef<boolean>(false);
  const isInCallRef = useRef<boolean>(false);

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
      isInCallRef.current = false;
    }
  }, []);

  const startPtt = useCallback(async () => {
    if (!sessionIdRef.current || !isInCallRef.current || pttActiveRef.current) return;
    pttActiveRef.current = true;
    setIsPttActive(true);
    try {
      await axios.post(`${apiUrl}/api/ptt/start`, {
        session_id: sessionIdRef.current,
      });
      appendLog('PTT activated — audio is being buffered.', 'system');
    } catch (err: unknown) {
      if (axios.isAxiosError(err)) {
        appendLog(`PTT start failed: ${err.response?.data?.status || err.message}`, 'system');
      } else {
        appendLog(`PTT start failed: ${err}`, 'system');
      }
      pttActiveRef.current = false;
      setIsPttActive(false);
    }
  }, [apiUrl, appendLog]);

  const stopPtt = useCallback(async () => {
    if (!sessionIdRef.current || !pttActiveRef.current) return;
    pttActiveRef.current = false;
    setIsPttActive(false);
    try {
      await axios.post(`${apiUrl}/api/ptt/stop`, {
        session_id: sessionIdRef.current,
      });
      appendLog('PTT released — buffered audio sent.', 'system');
    } catch (err: unknown) {
      if (axios.isAxiosError(err)) {
        appendLog(`PTT stop failed: ${err.response?.data?.status || err.message}`, 'system');
      } else {
        appendLog(`PTT stop failed: ${err}`, 'system');
      }
    }
  }, [apiUrl, appendLog]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.ctrlKey && e.code === 'Space') {
        e.preventDefault();
        if (!pttActiveRef.current && isInCallRef.current) {
          startPtt();
        }
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      if (e.code === 'Space' || e.key === 'Control') {
        if (pttActiveRef.current) {
          stopPtt();
        }
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [startPtt, stopPtt]);

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
            isInCallRef.current = true;
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

          if (answer.sdp) {
            answer.sdp = answer.sdp.replace(
              /(a=fmtp:\d+)(.*?)(?:\r\n|$)/,
              (_match, prefix, params) => {
                let p = params;
                const setParam = (key: string, value: string) => {
                  const regex = new RegExp(`;?${key}=\\w+`, 'g');
                  p = p.replace(regex, '');
                  p += `;${key}=${value}`;
                };
                setParam('cbr', '1');
                setParam('maxaveragebitrate', '64000');
                setParam('ptime', '20');
                setParam('minptime', '20');
                setParam('maxptime', '20');
                setParam('usedtx', '0');
                setParam('stereo', '0');
                setParam('sprop-stereo', '0');
                setParam('maxplaybackrate', '16000');
                setParam('sprop-maxcapturerate', '16000');
                return `${prefix}${p}\r\n`;
              }
            );

            if (!answer.sdp.includes('a=ptime:')) {
              answer.sdp = answer.sdp.replace(
                /(m=audio[^\r\n]*\r\n)/,
                '$1a=ptime:20\r\na=maxptime:20\r\n'
              );
            }
          }

          await pc.setLocalDescription(answer);

          const senders = pc.getSenders();
          for (const sender of senders) {
            if (sender.track?.kind === 'audio') {
              const params = sender.getParameters();
              if (params.encodings && params.encodings.length > 0) {
                params.encodings[0].maxBitrate = 64000;
                sender.setParameters(params).catch(() => {});
              }
            }
          }
          
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

  return { messages, isConnected, sessionId, isInCall, isPttActive, connectToServer, stopCall, connectToPeer };
};