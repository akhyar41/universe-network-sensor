import { browser } from "$app/env"
import { APP_URL } from "#lib/env"
import { current } from "#lib/global-date"
import { toast } from "svelte-sonner"

String.prototype.url = function (path: string | null = null): string {
  const base = APP_URL

  return path ? `${base}/${path}/${this}` : `${base}/${this}`
}

String.prototype.filename = function (): string {
  const clean = this.split(/[?#]/)[0]

  return clean.split(/[\\/]/).pop() || ""
}

String.prototype.fileExtension = function (): string {
  const url = this.split("?")[0]
  const index = url.lastIndexOf(".")

  return index > -1 ? url.substring(index + 1).toLowerCase() : ""
}

String.prototype.fileDownload = function (filename: string = "") {
  if (!browser) return

  const basename = filename || this.filename()
  const link = document.createElement("a")

  link.href = this.valueOf()
  link.download = basename || ""
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  URL.revokeObjectURL(link.href)
}

String.prototype.fileOpen = function (extDefault: string = "") {
  if (!browser) return

  const ext = extDefault || this.fileExtension()

  if (ext == "pdf") {
    window.dispatchEvent(new CustomEvent("view-pdf", { detail: this }))
    return
  }

  if (
    ext == "doc" ||
    ext == "docx" ||
    ext == "xls" ||
    ext == "xlsx"
  ) {
    window.dispatchEvent(new CustomEvent("view-doc", { detail: this }))
    return
  }

  if (
    ext == "image" ||
    ext == "jpg" ||
    ext == "jpeg" ||
    ext == "png" ||
    ext == "gif"
  ) {
    window.dispatchEvent(new CustomEvent("view-image", { detail: this }))
    return
  }

  toast.error("File not recognized")
}

String.prototype.linkOpen = function (): void {
  try {

    if (typeof _device == "undefined") {
      throw Error("This feature requires the app")
    }

    _device.open(this)

  } catch (e) {
    toast.info("Access is restricted", {
      description: e instanceof Error ? e.message : "Unknown error",
    })
  }
}

Blob.prototype.download = function (filename: string): void {
  if (!browser) return

  const url = URL.createObjectURL(this)
  const link = document.createElement("a")

  link.href = url
  link.download = filename
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  URL.revokeObjectURL(link.href)
}

Blob.prototype.fileOpenLog = function (filename: string = ""): void {
  if (!browser) return

  const url = URL.createObjectURL(this)

  window.dispatchEvent(new CustomEvent("view-log", {
    detail: {
      url,
      filename: filename || `${current().format("yyyy-MM-dd HH:mm:ss")}.log`
    }
  }))
}
