export function usePath() {

  const parts = window.location
                      .pathname
                      .split("/")
                      .filter(Boolean)
  const [moduleId, menuId, menuSubId] = parts

  return {
    name: window.location.pathname,
    path: [moduleId, menuId, menuSubId]
      .filter(Boolean)
      .join("/"),
    moduleId,
    menuId,
    menuSubId,
  }
}
