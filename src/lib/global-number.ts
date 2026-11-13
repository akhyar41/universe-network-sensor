Number.prototype.toBoolean = function (): boolean {
  return this.valueOf() === 1
}

Number.prototype.toUnitDistance = function (): string {

  if (this.valueOf() < 1000) {
    return `${this.valueOf().toFixed(0)}m`
  }

  return `${(this.valueOf() / 1000).toFixed(2)}Km`
}

Number.prototype.toUnitTime = function (): string {

  if (this.valueOf() < 60) {
    return `${this.valueOf().toFixed(0)} min`
  }

  return `${(this.valueOf() / 60).toFixed(2)} h`
}

Number.prototype.toNumber = function (): number {
  return Number(this)
}

Number.prototype.toMoney = function (): string {
  return Number(this).toLocaleString("en-US")
}
