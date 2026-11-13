const DIALOG_STACK_KEY = "__dialogStackTop"

// Open dialogs in opening order. Only the last entry owns the newest history entry, so only it
// may close through history.back().
const openDialogs: number[] = []

function removeFromDialogStack(index: number | null) {
  if (index == null) return
  const position = openDialogs.indexOf(index)
  if (position > -1) openDialogs.splice(position, 1)
}

export function useDialog(initialState = false) {

  let state = $state(initialState)
  let myIndex: number | null = null

  $effect(() => {
    if (!state) return

    const currentTop: number =
      window.history.state && typeof window.history.state[DIALOG_STACK_KEY] === "number"
        ? window.history.state[DIALOG_STACK_KEY]
        : 0
    const index = currentTop + 1
    myIndex = index
    openDialogs.push(index)

    const nextState = { ...(window.history.state ?? {}), [DIALOG_STACK_KEY]: index }
    window.history.pushState(nextState, "", window.location.href)

    function handlePopState(event: PopStateEvent) {

      const newTop: number = event.state && typeof event.state[DIALOG_STACK_KEY] === "number"
        ? event.state[DIALOG_STACK_KEY]
        : 0

      if (state && myIndex != null && myIndex > newTop) {
        myIndex = null
        state = false
      }
    }

    window.addEventListener("popstate", handlePopState)
    return () => {
      window.removeEventListener("popstate", handlePopState)
      removeFromDialogStack(index)
    }
  })

  function setOpen(newOpen: boolean) {
    if (newOpen === state) return
    if (newOpen) {
      state = true
      return
    }
    if (myIndex != null && openDialogs.at(-1) === myIndex) {
      window.history.back()
      return
    }
    // A dialog below the top must not pop the top one: close it in place instead.
    removeFromDialogStack(myIndex)
    myIndex = null
    state = false
  }

  return {
    get state() {
      return state
    },
    setOpen,
  }
}
