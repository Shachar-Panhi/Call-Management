import { useWebRTC } from './hooks/useWebRTC';
import { MessageList } from './components/MessageList';
import { MessageInput } from './components/MessageInput';

export default function App() {
  const { messages, isDataChannelOpen, sendMessage } = useWebRTC('ws://127.0.0.1:8080');

  return (
    <div style={{ fontFamily: 'sans-serif', padding: '20px', maxWidth: '600px', margin: '0 auto' }}>
      <h2>Signaling & WebRTC Console</h2>
      <MessageList messages={messages} />
      <MessageInput onSendMessage={sendMessage} disabled={!isDataChannelOpen} />
    </div>
  );
}