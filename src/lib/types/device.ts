export type DeviceMessage = {
  ulid?: string
  id: string
  name: string
  type: string
  value?: number
  ts: number
}

export type DeviceRecord = {
  key: string
  id: string
  name: string
  type: string
  last_value?: number
  ts: number
  last_seen_at: number
  online?: boolean
}

export type EventRecord = {
  id?: number
  device_id: string
  name: string
  type: string
  value?: number
  ts: number
  received_at: number
}

export type MqttStatus = "idle" | "connecting" | "connected" | "reconnecting" | "error"
