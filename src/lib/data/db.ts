import Dexie, { type Table } from "dexie"
import type { DeviceRecord, EventRecord } from "#lib/types/device"

// Keep the local history bounded so IndexedDB does not grow forever.
export const EVENT_LIMIT = 1000

export class DashboardDatabase extends Dexie {

  devices!: Table<DeviceRecord, string>
  events!: Table<EventRecord, number>

  constructor() {
    super("universe-network-sensor")

    this.version(1).stores({
      devices: "key, id, type, last_seen_at",
      events: "++id, received_at, device_id, type",
    })
  }
}

export const db = new DashboardDatabase()
