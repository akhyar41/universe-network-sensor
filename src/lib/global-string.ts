import { APP_URL } from "#lib/env"
import { toast } from "svelte-sonner"

String.prototype.toBoolean = function (): boolean {
  return this == "1" || this == "true"
}

String.prototype.toInt = function (): number {
  return +this
}

Number.prototype.toNumber = function (): number {
  return Number(this)
}

String.prototype.toNumber = function (): number {
  return Number(this.replace(/[^\d-]/g, ""))
}


String.prototype.toPhone = function (): string {

  const phone = this.replace(/[-+]/g, "")

  if (phone.startsWith("08")) {
    return phone.replace(/^08/, "628")
  }

  if (phone.startsWith("62")) {
    return phone
  }

  return phone
}


String.prototype.in = function (data: string[]): boolean {
  return data.includes(this.valueOf())
}

String.prototype.padLine = function (maxChar: number): string {

  let result = ""
  let currentLine = ""

  this
    .split(" ")
    .forEach(word => {
      if ((currentLine + word).length <= maxChar) {
        currentLine += (currentLine ? " " : "") + word
      } else {
        result += (result ? "\n" : "") + currentLine
        currentLine = word
      }
    })

  if (currentLine) {
    result += (result ? "\n" : "") + currentLine // Add the last line
  }

  return result
}

String.prototype.formatLine = function (maxChar: number): string {

  return ""
}

String.prototype.url = function (path: string | null = null): string {
  const base = APP_URL
  return path
    ? `${base}/${path}/${this}`
    : `${base}/${this}`
}

String.prototype.toUri = function (): string {
  return encodeURI(this as string)
}

String.prototype.fileExtension = function (): string {
  const url = this.split("?")[0]
  const index = url.lastIndexOf(".")
  return index > -1 ? url.substring(index + 1).toLowerCase() : ""
}

String.prototype.fileOpen = function (extDefault: string = "") {

  const ext = extDefault || this.fileExtension()

  if (
    ext == "media" ||
    ext == "jpg" ||
    ext == "jpeg" ||
    ext == "png" ||
    ext == "gif"
  ) {
    window.dispatchEvent(new CustomEvent("view-image", { detail: this }))
    return
  }

  toast.error("Unrecognized file")
}

String.prototype.linkOpen = function (): void {
  try {

    if (typeof _device == "undefined") {
      throw Error("This feature is available only in the app")
    }

    _device.open(this)

  } catch (e) {
    toast.info("Access restricted", {
      description: e instanceof Error ? e.message : "Unknown error.",
    })
  }
}
