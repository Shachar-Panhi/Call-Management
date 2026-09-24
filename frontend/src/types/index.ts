import { z } from 'zod';

export const ConnectionPacketSchema = z.object({
  session_id: z.string(),
});

export const SdpPacketSchema = z.object({
  session_id: z.string(),
  sdp: z.string(),
});

export const IcePacketSchema = z.object({
  session_id: z.string(),
  candidate: z.string(),
  sdpMid: z.string().nullable().optional(),
});

export type MessageType = 'sent' | 'received' | 'system';

export interface LogMessage {
  id: number;
  text: string;
  type: MessageType;
}