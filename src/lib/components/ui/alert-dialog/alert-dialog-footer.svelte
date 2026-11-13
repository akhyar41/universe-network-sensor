<script lang="ts">
  import { cn, type WithElementRef } from "#lib/utils"
  import type { HTMLAttributes } from "svelte/elements"

  let {
    ref = $bindable(null),
    class: className,
    children,
    ...restProps
  }: WithElementRef<HTMLAttributes<HTMLDivElement>> = $props()
</script>

<div
  bind:this={ref}
  data-slot="alert-dialog-footer"
  class={cn(
		"cn-alert-dialog-footer flex gap-2 flex-row justify-end",
		className
	)}
  {...restProps}
>
  {@render children?.()}
</div>
