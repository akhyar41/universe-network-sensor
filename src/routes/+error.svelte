<script lang="ts">
  import { page } from "$app/state"
  import { ErrorActionPage, ErrorStatusPage } from "#lib/components/page"
</script>

<svelte:head>
  <title>Error {page.status}</title>
</svelte:head>

{#if (page.status === 404)}
  <ErrorStatusPage
    title="Page not found"
    message="The page you are looking for was not found"
  />
{:else if (page.status === 403)}
  <ErrorStatusPage
    title="No access"
    message="You do not have access to this page"
  />
{:else if (page.status === 500)}
  <ErrorActionPage
    message="Failed to load the page. Please try again"
    useActionReload={true}
  />
{:else}
  <ErrorActionPage
    message={page.error?.message || "Failed to load the page. Please try again"}
    useActionReload={true}
  />
{/if}
