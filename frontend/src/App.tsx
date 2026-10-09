import { useState } from 'react';
import { useWebRTC } from './hooks/useWebRTC';
import { MessageList } from './components/MessageList';

export default function App() {
  const { messages, isConnected, sessionId, isInCall, connectToServer, stopCall, connectToPeer } = useWebRTC('ws://127.0.0.1:8080');
  const [targetId, setTargetId] = useState('');
  const [hasCopied, setHasCopied] = useState(false);

  const handleCopySessionId = () => {
    navigator.clipboard.writeText(sessionId);
    setHasCopied(true);
    setTimeout(() => setHasCopied(false), 2000);
  };

  return (
    <div className="max-w-2xl mx-auto p-5 font-sans">
      <h2 className="text-2xl font-bold text-gray-800 mb-4">PTT Audio Console</h2>
      
      {isConnected && sessionId && (
        <div className="mb-4 p-3 bg-blue-100 text-blue-900 rounded font-mono text-sm flex items-center justify-between">
          <div>
            Your Session ID: <strong>{sessionId}</strong>
          </div>
          <button 
            onClick={handleCopySessionId}
            className="px-3 py-1 bg-blue-200 hover:bg-blue-300 text-blue-800 font-semibold rounded transition-colors"
          >
            {hasCopied ? 'Copied' : 'Copy'}
          </button>
        </div>
      )}

      <MessageList messages={messages} />
      
      <div className="flex gap-3 mt-4">
        {!isConnected ? (
          <button 
            onClick={connectToServer}
            className="flex-1 px-5 py-3 bg-green-500 text-white font-medium rounded shadow hover:bg-green-600 transition-colors"
          >
            Connect to Server
          </button>
        ) : (
          <div className="flex flex-1 gap-2">
            <input 
              type="text" 
              value={targetId}
              onChange={(e) => setTargetId(e.target.value)}
              placeholder="Enter partner Session ID"
              disabled={isInCall}
              className="flex-1 px-4 py-2 border border-gray-300 rounded font-mono text-sm focus:outline-none focus:border-blue-500 disabled:bg-gray-100 disabled:cursor-not-allowed"
            />
            <button 
              onClick={() => connectToPeer(targetId)}
              disabled={!targetId || isInCall}
              className="px-5 py-3 bg-blue-500 text-white font-medium rounded shadow hover:bg-blue-600 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isInCall ? 'In Call' : 'Call'}
            </button>
            <button 
              onClick={stopCall}
              disabled={!isInCall}
              className="px-5 py-3 bg-red-500 text-white font-medium rounded shadow hover:bg-red-600 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
            >
              End Call
            </button>
          </div>
        )}
      </div>
    </div>
  );
}