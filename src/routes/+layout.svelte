<script lang="ts">

  import "./layout.css"
  import { Toaster } from "#lib/components/ui/sonner"
  import { APP_NAME } from "#lib/env"
  import { QueryClient, QueryClientProvider } from "@tanstack/svelte-query"
  import { ModeWatcher } from "mode-watcher"
  import { onMount } from "svelte"
  import ErrorBoundary from "./_components/error-boundary.svelte"

  import "#lib/env"
  import "#lib/global"

  let { children } = $props()

  const queryClient = new QueryClient({
    defaultOptions: {
      queries: {
        networkMode: "always",
        refetchOnWindowFocus: false,
        retry: false,
        staleTime: 0,
        gcTime: 0,
      }
    }
  })

  onMount(() => {

    document.getElementById("splash")?.remove()

  })

</script>

<svelte:head>
  <title>{APP_NAME}</title>
</svelte:head>

<ModeWatcher/>
<Toaster
  richColors
  position="top-center"
  theme="system"
/>
<ErrorBoundary>
  <QueryClientProvider client={queryClient}>
    {@render children()}
  </QueryClientProvider>
</ErrorBoundary>
