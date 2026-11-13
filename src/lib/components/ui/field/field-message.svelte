<script lang="ts">
  import { cn, type WithElementRef } from "#lib/utils"
  import { type AnyFormApi } from "@tanstack/svelte-form"
  import type { HTMLAttributes } from "svelte/elements"

  let {
    ref = $bindable(null),
    form,
    name,
    class: className,
    ...restProps
  }: WithElementRef<HTMLAttributes<HTMLDivElement>> & {
    form: AnyFormApi
    name: string
  } = $props()

  let message: string | null = $state(null)

  $effect(() => {

    const unsub = form.store.subscribe((state) => {

      const errors = state.fieldMeta[name]?.errors.map((error) => error.message) ?? []

      message = errors
        .filter(Boolean)
        .join(", ") || null
    })

    return () => unsub.unsubscribe()
  })
</script>

{#if message}
  <div
    bind:this={ref}
    role="alert"
    data-slot="field-message"
    class={cn("text-destructive text-xs font-normal", className)}
    {...restProps}
  >
    {message}
  </div>
{/if}
