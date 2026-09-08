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
  const wsRef = useRef<WebSocket | null>(null);
  const logEndRef = useRef<HTMLDivElement>(null);
  const messageIdRef = useRef<number>(0);

  const appendLog = (text: string, type: MessageType) => {
    setMessages((prev) => [
      ...prev,
      { id: messageIdRef.current++, text, type },
    ]);
  };

  useEffect(() => {
    const ws = new WebSocket('ws://127.0.0.1:8080');
    wsRef.current = ws;

    ws.onopen = () => {
      appendLog('Connected to server.', 'system');
    };

    ws.onmessage = (event) => {
      appendLog(event.data, 'received');
    };

    ws.onclose = () => {
      appendLog('Disconnected.', 'system');
    };

    ws.onerror = () => {
      appendLog('Connection error.', 'system');
    };

    return () => {
      ws.close();
    };
  }, []);

  useEffect(() => {
    if (logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages]);

  const sendMessage = () => {
    if (input && wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(input);
      appendLog(`Sent: ${input}`, 'sent');
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
    <div style={{ fontFamily: 'sans-serif', padding: '20px' }}>
      <div
        style={{
          border: '1px solid #ccc',
          height: '300px',
          overflowY: 'scroll',
          padding: '10px',
          marginBottom: '10px',
          textAlign: 'left'
        }}
      >
        {messages.map((msg) => (
          <div key={msg.id} style={getMessageStyle(msg.type)}>
            {msg.text}
          </div>
        ))}
        <div ref={logEndRef} />
      </div>
      <input
        type="text"
        value={input}
        onChange={(e) => setInput(e.target.value)}
        onKeyDown={handleKeyDown}
        placeholder="Type message..."
        autoFocus
        style={{ marginRight: '5px', padding: '5px' }}
      />
      <button onClick={sendMessage} style={{ padding: '5px 10px' }}>Send</button>
    </div>
  );
}