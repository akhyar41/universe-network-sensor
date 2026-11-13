import { liveQuery } from "dexie"
import { onMount } from "svelte"

export function useLiveQuery<T>(querier: () => Promise<T>, initial: T) {

  let data = $state<T>(initial)

  onMount(() => {
    const subscription = liveQuery(querier).subscribe({
      next: value => {
        data = value
      },
      error: error => {
        console.error("[DB] live query failed", error)
      },
    })

    return () => subscription.unsubscribe()
  })

  return {
    get data(): T {
      return data
    },
  }
}
