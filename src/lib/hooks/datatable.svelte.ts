export function createDataTable<T>({ initState, initSortBy, initSort, initLimit = 15 }: {
  initState: T
  initSortBy: string
  initSort: string
  initLimit?: number
}) {

  let extra = $state(initState)

  let sortBy = $state(initSortBy)
  let sort = $state(initSort)
  let total = $state(0)
  let limit = $state(initLimit)
  let pageCurrent = $state(1)
  let pageNonce = $state(0)

  const pageTotal = $derived(Math.max(1, Math.ceil(total / limit)))

  const start = $derived(total === 0 ? 0 : (pageCurrent - 1) * limit + 1)
  const end = $derived(Math.min(pageCurrent * limit, total))

  function goToPage(page: number | string) {
    pageCurrent = Math.max(1, Math.min(Number(page), pageTotal))
    pageNonce++
  }

  function setSort(by: string) {
    if (sortBy === by) {
      sort = sort === "asc" ? 'desc' : 'asc'
    } else {
      sortBy = by
      sort = 'asc'
    }
    pageCurrent = 1
  }

  function setExtra(key: string, value: any) {
    extra = { ...extra, [key]: value }
    pageCurrent = 1
  }

  return {
    get limit() {
      return limit
    },
    get start() {
      return start
    },
    get end() {
      return end
    },
    get total() {
      return total
    },
    get page() {
      void pageNonce
      return pageCurrent
    },
    get pageTotal() {
      return pageTotal
    },
    // actions
    setTotal(value: number | string) {
      total = value as number
    },
    setLimit(value: number | string) {
      limit = value as number
    },
    goToPage,
    nextPage() {
      goToPage(pageCurrent + 1)
    },
    prevPage() {
      goToPage(pageCurrent - 1)
    },
    setSort,
    get params() {
      return {
        sort_by: sortBy,
        sort: sort,
        limit: limit,
        page: pageCurrent,
        offset: limit * (pageCurrent - 1),
      }
    },
    get extra(): T {
      return extra
    },
    setExtra,
  }
}