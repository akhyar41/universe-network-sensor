
Boolean.prototype.toNumber = function (): number {
  return this.valueOf() ? 1 : 0
}
