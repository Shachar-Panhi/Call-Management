import { useWebRTC } from './hooks/useWebRTC';
import { MessageList } from './components/MessageList';

export default function App() {
  const { messages, isConnected, startCall, stopCall } = useWebRTC('ws://127.0.0.1:8080');

  return (
    <div className="max-w-2xl mx-auto p-5 font-sans">
      <h2 className="text-2xl font-bold text-gray-800 mb-4">PTT Audio Console</h2>
      
      <MessageList messages={messages} />
      
      <div className="flex gap-3 mt-4">
        {!isConnected ? (
          <button 
            onClick={startCall}
            className="flex-1 px-5 py-3 bg-green-500 text-white font-medium rounded shadow hover:bg-green-600 transition-colors"
          >
            Enable Microphone & Connect
          </button>
        ) : (
          <button 
            onClick={stopCall}
            className="flex-1 px-5 py-3 bg-red-500 text-white font-medium rounded shadow hover:bg-red-600 transition-colors"
          >
            Disconnect & Stop Microphone
          </button>
        )}
      </div>
    </div>
  );
}