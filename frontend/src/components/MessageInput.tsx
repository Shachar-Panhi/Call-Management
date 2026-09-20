import React, { useState } from 'react';

interface MessageInputProps {
  onSendMessage: (message: string) => void;
  disabled: boolean;
}

export const MessageInput: React.FC<MessageInputProps> = ({ onSendMessage, disabled }) => {
  const [input, setInput] = useState<string>('');

  const handleSend = () => {
    if (input.trim()) {
      onSendMessage(input);
      setInput('');
    }
  };

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      handleSend();
    }
  };

  return (
    <div style={{ display: 'flex' }}>
      <input
        type="text"
        value={input}
        onChange={(e) => setInput(e.target.value)}
        onKeyDown={handleKeyDown}
        placeholder={!disabled ? "Type message to send over WebRTC..." : "Connecting..."}
        disabled={disabled}
        autoFocus
        style={{ flexGrow: 1, marginRight: '10px', padding: '10px' }}
      />
      <button 
        onClick={handleSend} 
        disabled={disabled}
        style={{ padding: '10px 20px', cursor: !disabled ? 'pointer' : 'not-allowed' }}
      >
        Send
      </button>
    </div>
  );
};

