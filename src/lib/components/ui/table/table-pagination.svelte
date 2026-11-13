<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { Input } from "#lib/components/ui/input"
  import { Select, SelectContent, SelectItem, SelectTrigger } from "#lib/components/ui/select"
  import ChevronLeftIcon from "@lucide/svelte/icons/chevron-left"
  import ChevronRightIcon from "@lucide/svelte/icons/chevron-right"

  let {
    datatable
  }: {
    datatable: {
      limit: number
      start: number
      end: number
      total: number
      page: number
      pageTotal: number
      setLimit: (limit: number | string) => void
      goToPage: (page: string) => void
      prevPage: () => void
      nextPage: () => void
    }
  } = $props()

</script>

<div class="flex items-center justify-between gap-4">
  <Select
    type="single"
    value={datatable.limit.toString()}
    onValueChange={(value) => datatable.setLimit(value)}>
    <SelectTrigger class="w-20 font-mono select-none">
      {datatable.limit}
    </SelectTrigger>
    <SelectContent class="min-w-auto">
      {#each [10, 20, 50, 100, 250, 500, 1000] as n (n)}
        <SelectItem class="font-mono" value={n.toString()}>{n}</SelectItem>
      {/each}
    </SelectContent>
  </Select>
  <p class="flex-none text-sm text-muted-foreground font-mono">
    {datatable.start}–{datatable.end} of {datatable.total}
  </p>
  <div class="flex items-center gap-1">
    <Button variant="ghost"
            size="icon"
            onclick={() => datatable.prevPage()}
            disabled={datatable.page <= 1}>
      <ChevronLeftIcon/>
    </Button>
    <Input class="h-8 w-12 px-2 text-sm text-center font-mono"
           value={datatable.page}
           onchange={(e) => datatable.goToPage(e.currentTarget.value)}/>
    <Button variant="ghost"
            size="icon"
            onclick={() => datatable.nextPage()}
            disabled={datatable.page >= datatable.pageTotal}>
      <ChevronRightIcon/>
    </Button>
  </div>
</div>
