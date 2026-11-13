// See https://svelte.dev/docs/kit/types#app.d.ts
// for information about these interfaces

declare global {

  namespace App {
    // interface Error {}
    // interface Locals {}
    // interface PageData {}
    // interface PageState {}
    // interface Platform {}
  }

  const _device

}

declare module "@internationalized/date" {
  interface DateValue {
    format(pattern: string): string

    toDateJs(): Date

    isToday(): boolean

    isWeekday(): boolean

    isWeekend(): boolean

    isBefore(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean

    isAfter(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean
  }

  interface CalendarDate {
    format(pattern: string): string

    toDateJs(): Date

    isToday(): boolean

    isWeekday(): boolean

    isWeekend(): boolean

    isBefore(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean

    isAfter(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean
  }

  interface CalendarDateTime {
    format(pattern: string): string

    toDateJs(): Date

    isToday(): boolean

    isWeekday(): boolean

    isWeekend(): boolean

    isBefore(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean

    isAfter(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean
  }

  interface ZonedDateTime {
    format(pattern: string): string

    toDateJs(): Date

    isToday(): boolean

    isWeekday(): boolean

    isWeekend(): boolean

    isBefore(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean

    isAfter(value: CalendarDate | CalendarDateTime | ZonedDateTime): boolean
  }
}

export {}
