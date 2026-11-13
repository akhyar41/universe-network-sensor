<script lang="ts">
  import { ErrorActionPage } from "#lib/components/page"
  import { onMount } from "svelte"

  const { children } = $props()

  let error = $state<Error | null>(null)

  const handleError = (e: any) => {
    console.log(e)
    error = e
  }

  onMount(() => {

    window.addEventListener("error", handleError)
    window.addEventListener("unhandledrejection", handleError)

    return () => {
      window.removeEventListener("error", handleError)
      window.removeEventListener("unhandledrejection", handleError)
    }
  })

</script>

<svelte:boundary onerror={handleError}>
  {#if error}
    <ErrorActionPage
      message="The page failed to load. Please try again"
      useActionReload={true}
    />
  {:else}
    {@render children()}
  {/if}
</svelte:boundary>
