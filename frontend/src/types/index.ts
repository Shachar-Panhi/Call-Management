import { z } from 'zod';

export const SignalMessageSchema = z.union([
  z.object({
    type: z.literal('offer'),
    sdp: z.string(),
  }),
  z.object({
    type: z.literal('answer'),
    sdp: z.string(),
  }),
  z.object({
    type: z.literal('candidate'),
    candidate: z.string(),
    sdpMid: z.string().nullable().optional(),
  })
]);

export type SignalMessage = z.infer<typeof SignalMessageSchema>;

export type MessageType = 'sent' | 'received' | 'system';

export interface LogMessage {
  id: number;
  text: string;
  type: MessageType;
}

