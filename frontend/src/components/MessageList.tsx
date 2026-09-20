import React, { useEffect, useRef } from 'react';
import type { LogMessage, MessageType } from '../types';

interface MessageListProps {
  messages: LogMessage[];
}

export const MessageList: React.FC<MessageListProps> = ({ messages }) => {
  const logEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages]);

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
    <div
      style={{
        border: '1px solid #ccc',
        height: '400px',
        overflowY: 'scroll',
        padding: '10px',
        marginBottom: '10px',
        textAlign: 'left',
        backgroundColor: '#fafafa'
      }}
    >
      {messages.map((msg) => (
        <div key={msg.id} style={getMessageStyle(msg.type)}>
          {msg.text}
        </div>
      ))}
      <div ref={logEndRef} />
    </div>
  );
};
