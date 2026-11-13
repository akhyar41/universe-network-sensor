<script lang="ts">
  import { Dialog as DialogPrimitive } from "bits-ui"
  import { cn } from "#lib/utils"

  let {
    ref = $bindable(null),
    class: className,
    ...restProps
  }: DialogPrimitive.OverlayProps = $props()
</script>

<DialogPrimitive.Overlay
  bind:ref
  data-slot="dialog-overlay"
  class={cn("fixed grid place-items-center overflow-y-scroll py-4 inset-0 bg-black/20 duration-100 data-open:animate-in data-open:fade-in-0 data-closed:animate-out data-closed:fade-out-0 fixed inset-0 isolate z-50", className)}
  {...restProps}
/>
