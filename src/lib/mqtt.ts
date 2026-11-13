import { db, EVENT_LIMIT } from "#lib/data/db"
import type { DeviceMessage, DeviceRecord, MqttStatus } from "#lib/types/device"
import mqtt, { type MqttClient } from "mqtt"

export type MqttConnectionConfig = {
  url: string
  username?: string
  password?: string
  topicPrefixes: string[]
}

export type MqttConnectionHandlers = {
  onStatus(status: MqttStatus): void
  onError(message: string): void
  onMessage(at: number): void
}

type DeviceUpdate = {
  kind: "device"
  messages: DeviceMessage[]
  recordEvents: boolean
}

type StatusUpdate = {
  kind: "status"
  id: string
  name: string
  type: string
  online: boolean
  ts: number
}

type IncomingUpdate = DeviceUpdate | StatusUpdate

const NODE_TYPES: Record<string, true> = {
  door: true,
  alarm: true,
  gateway: true,
}

const STATE_VALUES: Record<string, number> = {
  OPEN: 1,
  ON: 1,
  ARMED: 1,
  TRUE: 1,
  CLOSED: 0,
  OFF: 0,
  DISARMED: 0,
  FALSE: 0,
}

function toDeviceMessage(value: unknown): DeviceMessage | null {
  if (typeof value !== "object" || value === null) return null

  const item = value as Record<string, unknown>
  if (typeof item.id !== "string" || typeof item.type !== "string") return null

  return {
    id: item.id,
    name: typeof item.name === "string" ? item.name : item.type,
    type: item.type,
    value: typeof item.value === "number" ? item.value : undefined,
    ts: typeof item.ts === "number" ? item.ts : Math.floor(Date.now() / 1000),
  }
}

function toDeviceMessages(data: unknown): DeviceMessage[] {
  const items = Array.isArray(data) ? data : [data]

  return items
    .map(toDeviceMessage)
    .filter((item): item is DeviceMessage => item !== null)
}

function readString(data: unknown, field: string): string | undefined {
  if (typeof data !== "object" || data === null) return undefined

  const value = (data as Record<string, unknown>)[field]
  return typeof value === "string" ? value : undefined
}

function readBoolean(data: unknown, field: string): boolean | undefined {
  if (typeof data !== "object" || data === null) return undefined

  const value = (data as Record<string, unknown>)[field]
  return typeof value === "boolean" ? value : undefined
}

function readTimestamp(data: unknown): number {
  if (typeof data !== "object" || data === null) return Math.floor(Date.now() / 1000)

  const value = (data as Record<string, unknown>).ts
  return typeof value === "number" ? value : Math.floor(Date.now() / 1000)
}

function toStateValue(state: string | undefined): number | undefined {
  if (!state) return undefined

  return STATE_VALUES[state.toUpperCase()]
}

function toStateMessage(id: string, name: string, type: string, data: unknown): DeviceMessage {
  return {
    id,
    name,
    type,
    value: toStateValue(readString(data, "state")),
    ts: readTimestamp(data),
  }
}

function parsePayload(payload: Uint8Array, topic: string): unknown {
  try {
    return JSON.parse(new TextDecoder().decode(payload))
  } catch (error) {
    console.warn(`[MQTT] ignored payload on '${topic}': invalid JSON`, error)
    return undefined
  }
}

function topicSuffix(topic: string, prefixes: string[]): string | null {
  for (const prefix of prefixes) {
    const base = prefix.replace(/\/+$/, "")
    if (topic === base) return ""
    if (topic.startsWith(`${base}/`)) return topic.slice(base.length + 1)
  }

  return null
}

function parseIncoming(topic: string, payload: Uint8Array, prefixes: string[]): IncomingUpdate | null {
  const suffix = topicSuffix(topic, prefixes)
  if (suffix === null) return null

  const data = parsePayload(payload, topic)
  if (data === undefined) return null

  if (suffix === "report" || suffix === "event") {
    return {
      kind: "device",
      messages: toDeviceMessages(data),
      recordEvents: suffix === "event",
    }
  }

  if (suffix === "door/state") {
    return {
      kind: "device",
      messages: [toStateMessage("node-door", "door", "door", data)],
      recordEvents: true,
    }
  }

  if (suffix === "alarm/state") {
    return {
      kind: "device",
      messages: [toStateMessage("node-alarm", "alarm", "alarm", data)],
      recordEvents: true,
    }
  }

  if (suffix === "system/mode") {
    return {
      kind: "device",
      messages: [{
        id: "node-gateway",
        name: "gateway",
        type: "gateway",
        value: toStateValue(readString(data, "mode")),
        ts: readTimestamp(data),
      }],
      recordEvents: false,
    }
  }

  const status = /^node\/([^/]+)\/status$/.exec(suffix)
  if (status) {
    const nodeId = status[1]
    const online = readBoolean(data, "online")

    if (online === undefined) return null

    return {
      kind: "status",
      id: `node-${nodeId}`,
      name: nodeId,
      type: NODE_TYPES[nodeId] ? nodeId : "node",
      online,
      ts: readTimestamp(data),
    }
  }

  return null
}

async function saveDevices(messages: DeviceMessage[]) {
  if (messages.length === 0) return

  const receivedAt = Date.now()

  const records: DeviceRecord[] = messages.map(message => ({
    key: `${message.id}:${message.type}`,
    id: message.id,
    name: message.name,
    type: message.type,
    last_value: message.value,
    ts: message.ts,
    last_seen_at: receivedAt,
    online: true,
  }))

  await db.devices.bulkPut(records)
}

async function saveStatus(update: StatusUpdate) {
  const receivedAt = Date.now()
  const key = `${update.id}:${update.type}`

  const changed = await db.devices.update(key, {
    ts: update.ts,
    last_seen_at: receivedAt,
    online: update.online,
  })

  if (changed === 0) {
    await db.devices.add({
      key,
      id: update.id,
      name: update.name,
      type: update.type,
      ts: update.ts,
      last_seen_at: receivedAt,
      online: update.online,
    })
  }
}

async function saveEvents(messages: DeviceMessage[]) {
  if (messages.length === 0) return

  const receivedAt = Date.now()

  await db.events.bulkAdd(messages.map(message => ({
    device_id: message.id,
    name: message.name,
    type: message.type,
    value: message.value,
    ts: message.ts,
    received_at: receivedAt,
  })))

  const staleKeys = await db.events
                                .orderBy("received_at")
                                .reverse()
                                .offset(EVENT_LIMIT)
                                .primaryKeys()

  if (staleKeys.length > 0) {
    await db.events.bulkDelete(staleKeys)
  }
}

// Serialize writes so report/event bursts cannot interleave partial state.
let ingestQueue = Promise.resolve()

function ingest(update: IncomingUpdate) {
  ingestQueue = ingestQueue
    .then(async () => {
      if (update.kind === "status") {
        await saveStatus(update)
        return
      }

      await saveDevices(update.messages)
      if (update.recordEvents) await saveEvents(update.messages)
    })
    .catch(error => console.error("[DB] failed to store MQTT message", error))
}

export function createMqttConnection(config: MqttConnectionConfig, handlers: MqttConnectionHandlers): MqttClient {
  const client = mqtt.connect(config.url, {
    clientId: `wsn-dashboard-${Math.random().toString(16).slice(2, 10)}`,
    clean: true,
    connectTimeout: 10_000,
    reconnectPeriod: 3_000,
    username: config.username || undefined,
    password: config.password || undefined,
  })

  const topics = config.topicPrefixes.map(prefix => `${prefix.replace(/\/+$/, "")}/#`)

  handlers.onStatus("connecting")

  client.on("connect", () => {
    handlers.onStatus("connected")

    client.subscribe(topics, error => {
      if (error) handlers.onError(`Subscribe failed: ${error.message}`)
    })
  })

  client.on("reconnect", () => handlers.onStatus("reconnecting"))

  client.on("error", error => {
    handlers.onStatus("error")
    handlers.onError(error.message)
  })

  client.on("message", (topic, payload) => {
    const update = parseIncoming(topic, payload, config.topicPrefixes)
    if (!update) return

    handlers.onMessage(Date.now())
    ingest(update)
  })

  return client
}
