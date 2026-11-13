import { browser } from "$app/env"

if (browser) {

  Document.prototype.keyEsc = function () {
    this.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape" }))
  }

}
