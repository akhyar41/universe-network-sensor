<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { APP_CODE, APP_VERSION } from "#lib/env"

  const handleRefresh = () => {
    window.location.reload()
  }

</script>
<main class="relative flex h-full w-full flex-col items-center justify-center">
  <div class="absolute top-4 inset-e-4 text-end font-mono text-xs font-light text-gray-800">
    <div>{APP_CODE}-{APP_VERSION}</div>
  </div>
  <div class="flex flex-col w-80 space-y-6 text-center">
    <div class="flex justify-center gap-2">
      <Button
        variant="outline"
        class="rounded-full"
        onclick={handleRefresh}>
        Reload
      </Button>
    </div>
  </div>
</main>
