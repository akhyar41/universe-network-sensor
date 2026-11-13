<script lang="ts">
  import { cn } from "#lib/utils"
  import type { HTMLAttributes } from "svelte/elements"

  let {
    src,
    alt,
    "alt-fallback": altFallback,
    class: className,
    ...restProps
  }: {
    src?: string
    alt?: string
    "alt-fallback"?: string
    class?: string
  } & HTMLAttributes<HTMLDivElement> = $props()

  let imgSrc = $derived(src || undefined)
  let isError = $derived(!src)

  $effect(() => {
    imgSrc = src
    isError = !src
  })

</script>
<div
  class={cn(
    "relative overflow-hidden flex items-center justify-center rounded-sm bg-muted",
    className
  )}
  {...restProps}>
  {#if !isError}
    <img
      src={imgSrc}
      alt={alt}
      class="absolute h-full w-full min-w-fit object-contain"
      onerror={() => {
        imgSrc = undefined
        isError = true
      }}/>
  {:else}
    <span class="text-xs">{altFallback || alt}</span>
  {/if}
</div>
