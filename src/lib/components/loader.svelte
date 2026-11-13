<script lang="ts">

  import { cn } from "#lib/utils"
  import LoaderCircleIcon from "@lucide/svelte/icons/loader-circle"

  const {
    show,
    className
  }: {
    show: boolean
    className?: string
  } = $props()

</script>

{#if show}
  <div class={cn(
  "absolute inset-s-0 top-0 w-full h-full flex items-center justify-center bg-background/50 rounded-lg z-1",
  className
)}>
    <LoaderCircleIcon class="size-10 animate-spin"/>
  </div>
{/if}
