import {
  CalendarDate,
  CalendarDateTime,
  type DateValue, fromAbsolute,
  getLocalTimeZone,
  isSameDay,
  isWeekday,
  isWeekend,
  now,
  parseDate,
  parseDateTime,
  parseTime,
  parseZonedDateTime,
  today,
  ZonedDateTime
} from "@internationalized/date"

export function current() {
  return now(getLocalTimeZone())
}

function _format(this: CalendarDate | CalendarDateTime | ZonedDateTime, pattern: string): string {

  const pad = (n: number) => String(n).padStart(2, "0")

  const year = this.year
  const month = this.month
  const day = this.day
  const hour = "hour" in this ? this.hour : 0
  const minute = "minute" in this ? this.minute : 0
  const second = "second" in this ? this.second : 0
  const millisecond = "millisecond" in this ? this.millisecond : 0

  const monthNames = [
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  ]
  const monthShort = monthNames.map(m => m.slice(0, 3))

  const dayNames = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
  const dayOfWeek = new Date(year, month - 1, day).getDay()

  const hour12 = hour % 12 || 12
  const ampm = hour < 12 ? "AM" : "PM"

  const tokens: Record<string, string> = {
    // day of week
    "EEEE": dayNames[dayOfWeek],
    // year
    "yyyy": String(year),
    "yyy": String(year).slice(1),
    "yy": String(year).slice(-2),
    "y": String(year),
    // month
    "MMMM": monthNames[month - 1],
    "MMM": monthShort[month - 1],
    "MM": pad(month),
    "M": String(month),
    // day
    "dd": pad(day),
    "d": String(day),
    // hour 24
    "HH": pad(hour),
    "H": String(hour),
    // hour 12
    "hh": pad(hour12),
    "h": String(hour12),
    // minute
    "mm": pad(minute),
    "m": String(minute),
    // second
    "ss": pad(second),
    "s": String(second),
    // millisecond
    "SSS": String(millisecond).padStart(3, "0"),
    "SS": String(millisecond).padStart(2, "0").slice(0, 2),
    "S": String(millisecond).slice(0, 1),
    // am/pm
    "A": ampm,
    "a": ampm.toLowerCase(),
  }

  const regex = new RegExp(
    Object.keys(tokens)
          .sort((a, b) => b.length - a.length)
          .join("|"),
    "g"
  )

  return pattern.replace(regex, (match) => tokens[match] ?? match)
}

function _toDateJs(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  timeZone: string | null = null
) {
  return this.toDate(timeZone || getLocalTimeZone())
}

function _isToday(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  timeZone: string | null = null
) {
  return isSameDay(this, today(timeZone || getLocalTimeZone()))
}

function _isWeekday(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  locale: string = "en-US",
) {
  return isWeekday(this, locale)
}

function _isWeekend(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  locale: string = "en-US",
) {
  return isWeekend(this, locale)
}

function _isBefore(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  value: CalendarDate | CalendarDateTime | ZonedDateTime
) {
  return this < value
}

function _isAfter(
  this: CalendarDate | CalendarDateTime | ZonedDateTime,
  value: CalendarDate | CalendarDateTime | ZonedDateTime
) {
  return this > value
}

// Number

Number.prototype.toDate = function(timeZone: string | null = null): DateValue {
  return fromAbsolute(this.valueOf() * 1000, timeZone || getLocalTimeZone())
}

// String

String.prototype.toDate = function (pattern: string | null = "yyyy-MM-dd"): DateValue {

  const str = this.valueOf()

  if (!str) return current()

  if (!pattern) {
    return parseZonedDateTime(str)
  }

  switch (pattern) {
    case "yyyy-MM-dd":
      return parseDate(str)

    case "yyyy-MM-dd'T'HH:mm":
    case "yyyy-MM-dd'T'HH:mm:ss":
      return parseDateTime(str)

    case "yyyy-MM-dd HH:mm":
    case "yyyy-MM-dd HH:mm:ss":
      return parseDateTime(str.replace(" ", "T"))

    default:
      return parseDate(str)
  }
}

String.prototype.toTime = function (): DateValue {

  const str = this.valueOf()

  if (!str) return current()

  return parseTime(str) as unknown as DateValue
}

String.prototype.toDateTime = function (pattern: string | null = "yyyy-MM-dd HH:mm:ss"): DateValue {

  const str = this.valueOf()

  if (!str) return current()

  if (!pattern) {
    return parseZonedDateTime(str)
  }

  switch (pattern) {
    case "yyyy-MM-dd":
      return parseDate(str)

    case "yyyy-MM-dd'T'HH:mm":
    case "yyyy-MM-dd'T'HH:mm:ss":
      return parseDateTime(str)

    case "yyyy-MM-dd HH:mm":
    case "yyyy-MM-dd HH:mm:ss":
      return parseDateTime(str.replace(" ", "T"))

    default:
      return parseDate(str)
  }
}

// DateValue

CalendarDate.prototype.format = _format
CalendarDateTime.prototype.format = _format
ZonedDateTime.prototype.format = _format

CalendarDate.prototype.toDateJs = _toDateJs
CalendarDateTime.prototype.toDateJs = _toDateJs
ZonedDateTime.prototype.toDateJs = _toDateJs

CalendarDate.prototype.isToday = _isToday
CalendarDateTime.prototype.isToday = _isToday
ZonedDateTime.prototype.isToday = _isToday

CalendarDate.prototype.isWeekday = _isWeekday
CalendarDateTime.prototype.isWeekday = _isWeekday
ZonedDateTime.prototype.isWeekday = _isWeekday

CalendarDate.prototype.isWeekend = _isWeekend
CalendarDateTime.prototype.isWeekend = _isWeekend
ZonedDateTime.prototype.isWeekend = _isWeekend

CalendarDate.prototype.isBefore = _isBefore
CalendarDateTime.prototype.isBefore = _isBefore
ZonedDateTime.prototype.isBefore = _isBefore

CalendarDate.prototype.isAfter = _isAfter
CalendarDateTime.prototype.isAfter = _isAfter
ZonedDateTime.prototype.isAfter = _isAfter
