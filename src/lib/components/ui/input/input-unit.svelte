<script lang="ts">

  import { cn } from "#lib/utils"
  import type { HTMLInputAttributes } from "svelte/elements"

  // Borderless, Excel-like number cell: the unit is a muted prefix on the
  // leading side and the right-aligned number pins to the cell's trailing edge,
  // so digits line up across rows. The cell relies on the surrounding table
  // gridlines and shows an inset ring only while focused.
  let {
    value,
    unit = "",
    onValue,
    class: className,
    disabled = false,
    ...restProps
  }: Omit<HTMLInputAttributes, "value" | "type"> & {
    value: number
    unit?: string
    onValue?: (value: number) => void
  } = $props()

</script>

<div
  data-disabled={disabled}
  class={cn(
    "flex h-7 items-stretch overflow-hidden bg-transparent transition-colors focus-within:ring-2 focus-within:ring-ring/60 focus-within:ring-inset data-[disabled=true]:opacity-50 data-[disabled=true]:pointer-events-none",
    className
  )}>
  {#if unit}
    <span class="flex select-none items-center pe-1 text-[10px] font-medium text-muted-foreground">
      {unit}
    </span>
  {/if}
  <input
    type="number"
    min="0"
    {value}
    {disabled}
    oninput={(e) => onValue?.(e.currentTarget.value.toNumber())}
    class="w-full min-w-0 bg-transparent px-1.5 text-end font-mono tabular-nums outline-none disabled:cursor-not-allowed md:text-sm"
    {...restProps}/>
</div>
