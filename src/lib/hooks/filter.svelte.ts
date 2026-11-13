export function createFilter<T>(initState: T) {

  let state = $state(initState)

  function set(key: string, value: unknown) {
    state = { ...state, [key]: value }
  }

  return {
    get state(): T {
      return state
    },
    set,
  }
}