<script lang="ts">
  import { Badge } from "#lib/components/ui/badge"
  import * as Card from "#lib/components/ui/card"
  import { db } from "#lib/data/db"
  import * as Empty from "#lib/components/ui/empty"
  import * as Table from "#lib/components/ui/table"
  import { APP_NAME } from "#lib/env"
  import { useLiveQuery } from "#lib/hooks/use-live-query.svelte"
  import type { DeviceRecord } from "#lib/types/device"

  const OFFLINE_AFTER_MS = 90_000
  const EVENT_ROWS = 100

  const devices = useLiveQuery(() => db.devices.toArray(), [])
  const events = useLiveQuery(
    () => db.events.orderBy("received_at").reverse().limit(EVENT_ROWS).toArray(),
    [],
  )

  let now = $state(Date.now())

  $effect(() => {
    const timer = setInterval(() => now = Date.now(), 1_000)

    return () => clearInterval(timer)
  })

  function isOnline(device: DeviceRecord): boolean {
    if (device.online === false) return false
    return now - device.last_seen_at <= OFFLINE_AFTER_MS
  }

  function formatValue(type: string, value?: number): string {
    if (value === undefined) return "—"

    if (type === "sensor-temperature") return `${value.toFixed(1)} °C`
    if (type === "sensor-humidity") return `${value.toFixed(1)} %`

    if (type === "door" || type === "button") {
      return ["CLOSED", "OPEN", "TRIGGERED"][value] ?? String(value)
    }

    if (type === "gateway") {
      return ["DISARMED", "ARMED"][value] ?? String(value)
    }

    if (value === 0) return "OFF"
    if (value === 1) return "ON"

    return String(value)
  }

  function formatTime(at: number): string {
    return new Date(at).toLocaleString("en-GB", {hour12: false})
  }

  function formatAge(at: number): string {
    const seconds = Math.max(0, Math.round((now - at) / 1_000))

    if (seconds < 60) return `${seconds}s ago`

    const minutes = Math.floor(seconds / 60)
    if (minutes < 60) return `${minutes}m ago`

    const hours = Math.floor(minutes / 60)
    if (hours < 24) return `${hours}h ago`

    return `${Math.floor(hours / 24)}d ago`
  }

</script>

<svelte:head>
  <title>Dashboard — {APP_NAME}</title>
</svelte:head>

<main class="w-full h-full overflow-auto scrollbar-gutter-stable">
  <div class="container mx-auto space-y-4 p-4">
    <div class="space-y-1">
      <h1 class="text-2xl font-bold">Live Monitor</h1>
      <p class="text-sm text-muted-foreground">
        Device reports and door events received over MQTT, stored locally with Dexie.
      </p>
    </div>

    <Card.Root>
      <Card.Header>
        <Card.Title>Devices</Card.Title>
        <Card.Description>
          {devices.data.length} device{devices.data.length === 1 ? "" : "s"} seen
        </Card.Description>
      </Card.Header>
      <Card.Content>
        {#if devices.data.length === 0}
          <Empty.Root>
            <Empty.Header>
              <Empty.Title>No devices yet</Empty.Title>
              <Empty.Description>
                Waiting for the first report or state message from a node.
              </Empty.Description>
            </Empty.Header>
          </Empty.Root>
        {:else}
          <div class="overflow-x-auto">
            <Table.Root>
              <Table.Header>
                <Table.Row>
                  <Table.Head class="text-start">Device</Table.Head>
                  <Table.Head class="text-start">Name</Table.Head>
                  <Table.Head class="text-start">Type</Table.Head>
                  <Table.Head class="text-start">Value</Table.Head>
                  <Table.Head class="text-start">Status</Table.Head>
                  <Table.Head class="text-start">Last seen</Table.Head>
                </Table.Row>
              </Table.Header>
              <Table.Body>
                {#each devices.data as device (device.key)}
                  <Table.Row>
                    <Table.Cell class="font-mono text-xs max-w-40 truncate" title={device.id}>
                      {device.id}
                    </Table.Cell>
                    <Table.Cell>{device.name}</Table.Cell>
                    <Table.Cell>{device.type}</Table.Cell>
                    <Table.Cell>{formatValue(device.type, device.last_value)}</Table.Cell>
                    <Table.Cell>
                      <Badge variant={isOnline(device) ? "success" : "inactive"}>
                        {isOnline(device) ? "Online" : "Offline"}
                      </Badge>
                    </Table.Cell>
                    <Table.Cell class="text-muted-foreground">
                      {formatAge(device.last_seen_at)}
                    </Table.Cell>
                  </Table.Row>
                {/each}
              </Table.Body>
            </Table.Root>
          </div>
        {/if}
      </Card.Content>
    </Card.Root>

    <Card.Root>
      <Card.Header>
        <Card.Title>Events</Card.Title>
        <Card.Description>
          Latest {EVENT_ROWS} events, newest first
        </Card.Description>
      </Card.Header>
      <Card.Content>
        {#if events.data.length === 0}
          <Empty.Root>
            <Empty.Header>
              <Empty.Title>No events yet</Empty.Title>
              <Empty.Description>
                Waiting for an event or state change from a node.
              </Empty.Description>
            </Empty.Header>
          </Empty.Root>
        {:else}
          <div class="overflow-x-auto">
            <Table.Root>
              <Table.Header>
                <Table.Row>
                  <Table.Head>Time</Table.Head>
                  <Table.Head>Device</Table.Head>
                  <Table.Head>Name</Table.Head>
                  <Table.Head>Type</Table.Head>
                  <Table.Head>Value</Table.Head>
                </Table.Row>
              </Table.Header>
              <Table.Body>
                {#each events.data as event (event.id)}
                  <Table.Row>
                    <Table.Cell class="text-muted-foreground">
                      {formatTime(event.received_at)}
                    </Table.Cell>
                    <Table.Cell class="font-mono text-xs max-w-40 truncate" title={event.device_id}>
                      {event.device_id}
                    </Table.Cell>
                    <Table.Cell>{event.name}</Table.Cell>
                    <Table.Cell>{event.type}</Table.Cell>
                    <Table.Cell>{formatValue(event.type, event.value)}</Table.Cell>
                  </Table.Row>
                {/each}
              </Table.Body>
            </Table.Root>
          </div>
        {/if}
      </Card.Content>
    </Card.Root>
  </div>
</main>
