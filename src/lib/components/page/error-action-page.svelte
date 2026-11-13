<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { APP_CODE, APP_VERSION } from "#lib/env"
  import ArrowLeftIcon from "@lucide/svelte/icons/arrow-left"

  const {
    message,
    useActionHome = false,
    useActionReload = false,
    useAction = false,
    action
  }: {
    message: string,
    useActionHome?: boolean,
    useActionReload?: boolean,
    useAction?: boolean,
    action?: () => void
  } = $props()

  const handleRefresh = () => {
    window.location.reload()
  }

</script>
<main class="relative flex h-full w-full flex-col items-center justify-center">
  <div class="absolute top-4 inset-e-4 text-end font-mono text-xs font-light text-gray-800">
    <div>{APP_CODE}-{APP_VERSION}</div>
  </div>
  <div class="flex w-80 flex-col text-center space-y-6">
    <div>
      <h3 class="text-xl font-bold text-primary">
        Something went wrong
      </h3>
      <p class="text-sm text-muted-foreground">
        {message}
      </p>
    </div>
    <div class="flex justify-center gap-2">
      {#if (useActionHome)}
        <Button href="/"
                variant="outline"
                class="flex-1 rounded-full">
          <ArrowLeftIcon/>
          Back to Home
          <div></div>
        </Button>
      {/if}
      {#if (useActionReload)}
        <Button
          variant="outline"
          class="rounded-full"
          onclick={handleRefresh}>
          Reload
        </Button>
      {/if}
      {#if (useAction)}
        <Button
          variant="outline"
          class="flex-1 rounded-full"
          onclick={action}>
          Retry
        </Button>
      {/if}
    </div>
  </div>
</main>
