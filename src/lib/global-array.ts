import * as _ from "lodash"
import { type CollectionChain } from "lodash"

Array.prototype.count = function (): number {
  return this.length
}

Array.prototype.indexMax = function (): number {
  return this.length - 1
}

Array.prototype.isEmpty = function <T>(this: T[]): boolean {
  return this.length === 0
}

Array.prototype.isNotEmpty = function <T>(this: T[]): boolean {
  return this.length !== 0
}

Array.prototype.toCollect = function <T>(): CollectionChain<T> {
  const array = this as T[]
  return _.chain(array)
}

Array.prototype.when = function (condition, callback) {
  if (condition) {
    callback(this)
  }
  return this
}

Array.prototype.findAndApply = function <T, R>(
  predicate: (value: T) => boolean,
  transform: (value: T) => R
): R | undefined {
  const found = this.find(predicate)
  return found !== undefined ? transform(found) : undefined
}
