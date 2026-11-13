import { type DateValue } from "@internationalized/date"
import { type CollectionChain } from "lodash"

import "#lib/global-boolean"
import "#lib/global-number"
import "#lib/global-string"
import "#lib/global-array"
import "#lib/global-date"
import "#lib/global-file"
import "#lib/global-keyboard"

declare global {

  interface Boolean {
    toNumber(): number
  }

  interface Number {
    toBoolean(): boolean

    toNumber(): number

    toMoney(): string

    toUnitDistance(): string

    toUnitTime(): string

    toDate(): DateValue

    toDate(timeZone: string): DateValue
  }

  interface String {
    toBoolean(): boolean

    toInt(): number

    toNumber(): number

    toNumeric(): string

    toMoney(): string

    toJSON(): unknown

    toDate(): DateValue

    toDate(pattern: string): DateValue

    toTime(): DateValue

    toDateTime(): DateValue

    toPhone(): string

    in(data: string[]): boolean

    padLine(maxChar: number): string

    formatLine(maxChar: number): string

    toUri(): string

    url(): string

    url(path: string | null): string

    filename(): string

    fileExtension(): string

    linkOpen(): void

    fileOpen(): void

    fileOpen(ext: string): void

    fileDownload(): void

    fileDownload(filename: string): void

  }

  interface Array<T> {
    count(): number

    indexMax(): number

    isEmpty(): boolean

    isNotEmpty(): boolean

    when(condition: boolean, callback: (array: T[]) => void): T[]

    findAndApply<R>(predicate: (value: T) => boolean, transform: (value: T) => R): R | undefined

    toCollect(): CollectionChain<T>
  }

  interface Date {

    format(format: string): string

    isToday(): boolean

    isWeekend(): boolean

    isBefore(date: Date): boolean

    isAfter(date: Date): boolean
  }

  interface Object {

    length(): number

    isEmpty(): boolean
  }

  interface Blob {
    download(filename: string): void

    fileOpenLog(): void

    fileOpenLog(filename: string): void
  }

  interface Document {

    keyEsc(): void

  }

}

Object.defineProperty(Object.prototype, "length", {
  value: function (): number {
    return Object.keys(this).length
  },
  writable: true,
  configurable: true,
})

Object.defineProperty(Object.prototype, "isEmpty", {
  value: function (): boolean {
    if (this == null) return true
    return Object.keys(this).length === 0
  },
  writable: true,
  configurable: true,
})
