<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { cn } from "#lib/utils"
  import ChevronsUpDownIcon from "@lucide/svelte/icons/chevrons-up-down"
  import { Popover as PopoverPrimitive } from "bits-ui"

  let {
    ref = $bindable(null),
    class: className,
    value,
    placeholder = "Select Data",
    ...restProps
  }: PopoverPrimitive.TriggerProps = $props()

</script>

<PopoverPrimitive.Trigger
  bind:ref
  data-slot="popover-trigger"
  {...restProps}>
  {#snippet child({ props })}
    <Button
      variant="outline"
      class={cn("w-full flex justify-between", className)}
      role="combobox"
      {...props}>
      <span class="min-w-0 truncate">{value || placeholder}</span>
      <ChevronsUpDownIcon class="opacity-50"/>
    </Button>
  {/snippet}
</PopoverPrimitive.Trigger>
