import React from 'react';
import { useWebRTC } from './hooks/useWebRTC';
import { MessageList } from './components/MessageList';

export default function App() {
  const { messages, isConnected, startCall, stopCall } = useWebRTC('ws://127.0.0.1:8080');

  return (
    <div style={{ fontFamily: 'sans-serif', padding: '20px', maxWidth: '600px', margin: '0 auto' }}>
      <h2>PTT Audio Console</h2>
      <MessageList messages={messages} />
      
      <div style={{ display: 'flex', gap: '10px', marginTop: '10px' }}>
        {!isConnected ? (
          <button 
            onClick={startCall}
            style={{ 
              padding: '10px 20px', 
              cursor: 'pointer', 
              backgroundColor: '#4CAF50', 
              color: 'white', 
              border: 'none', 
              borderRadius: '4px',
              flexGrow: 1
            }}
          >
            Enable Microphone & Connect
          </button>
        ) : (
          <button 
            onClick={stopCall}
            style={{ 
              padding: '10px 20px', 
              cursor: 'pointer', 
              backgroundColor: '#f44336', 
              color: 'white', 
              border: 'none', 
              borderRadius: '4px',
              flexGrow: 1
            }}
          >
            Disconnect & Stop Microphone
          </button>
        )}
      </div>
    </div>
  );
}