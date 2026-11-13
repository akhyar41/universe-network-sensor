<script lang="ts">
  import { Badge, type BadgeVariant } from "#lib/components/ui/badge"
  import { APP_NAME, APP_VERSION } from "#lib/env"
  import { mqtt } from "#lib/hooks/use-mqtt.svelte"
  import type { MqttStatus } from "#lib/types/device"
  import { onMount } from "svelte"

  let { children } = $props()

  const STATUS: Record<MqttStatus, { label: string, variant: BadgeVariant }> = {
    idle: {label: "Idle", variant: "secondary"},
    connecting: {label: "Connecting", variant: "outline"},
    connected: {label: "Live", variant: "success"},
    reconnecting: {label: "Reconnecting", variant: "outline"},
    error: {label: "Error", variant: "destructive"},
  }

  const status = $derived(STATUS[mqtt.status])

  onMount(() => {
    mqtt.start()

    return () => mqtt.stop()
  })

</script>

<div class="flex w-full h-dvh flex-col">
  <header class="flex items-center justify-between border-b px-4 py-3 shrink-0">
    <div class="flex items-baseline gap-2">
      <span class="font-semibold text-base">{APP_NAME}</span>
      <span class="text-xs text-muted-foreground">v{APP_VERSION}</span>
    </div>
    <Badge variant={status.variant} title={mqtt.error ?? undefined}>
      {status.label}
    </Badge>
  </header>
  <div class="min-h-0 flex-1">
    {@render children()}
  </div>
</div>
