import React, { useRef, useEffect } from 'react';
import type { LogMessage } from '../types';

interface MessageListProps {
  messages: LogMessage[];
}

export const MessageList: React.FC<MessageListProps> = ({ messages }) => {
  const endRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    endRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  return (
    <div className="h-96 overflow-y-auto bg-gray-900 border border-gray-800 rounded-xl p-6 shadow-inner font-mono text-sm flex flex-col space-y-1.5">
      {messages.map((msg) => (
        <div 
          key={msg.id} 
          className={`break-words leading-relaxed ${
            msg.type === 'system' ? 'text-blue-400' :
            msg.type === 'received' ? 'text-gray-300' :
            'text-emerald-400'
          }`}
        >
          <span className="text-gray-600 mr-3 opacity-75">{'>'}</span>
          {msg.text}
        </div>
      ))}
      <div ref={endRef} />
    </div>
  );
};