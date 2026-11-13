<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { cn, type WithElementRef } from "#lib/utils"
  import ArrowDownIcon from "@lucide/svelte/icons/arrow-down"
  import ArrowUpIcon from "@lucide/svelte/icons/arrow-up"
  import ChevronsUpDownIcon from "@lucide/svelte/icons/chevrons-up-down"
  import type { Snippet } from "svelte"
  import type { HTMLThAttributes } from "svelte/elements"

  let {
    ref = $bindable(null),
    class: className,
    children,
    name,
    datatable,
    ...restProps
  }: WithElementRef<HTMLThAttributes> & {
    children?: Snippet
    name: string
    datatable: {
      setSort: (by: string) => void
      params: {
        sort_by: string
        sort: string
      }
    }
  } = $props()

  const params = $derived(datatable.params)

</script>

<th
  bind:this={ref}
  data-slot="table-head"
  class={cn("px-2 align-middle text-foreground font-medium whitespace-nowrap has-[[role=checkbox]]:pr-0", className)}
  {...restProps}
>
  <Button
    variant="ghost"
    size="sm"
    class="h-7 gap-0.5 px-1.5 text-sm -mx-2"
    onclick={() => datatable.setSort(name)}
  >
    {@render children?.()}
    {#if params.sort_by !== name}
      <ChevronsUpDownIcon class="size-3.5"/>
    {:else if params.sort === "asc"}
      <ArrowUpIcon class="size-3.5"/>
    {:else}
      <ArrowDownIcon class="size-3.5"/>
    {/if}
  </Button>
</th>
