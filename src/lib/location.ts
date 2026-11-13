export function measureDistance(fromLatitude: number, fromLongitude: number, toLatitude: number, toLongitude: number): number {

  const R = 6371e3
  const fromLatitudeRadians = fromLatitude * Math.PI / 180
  const toLatitudeRadians = toLatitude * Math.PI / 180
  const deltaLatitude = (toLatitude - fromLatitude) * Math.PI / 180
  const deltaLongitude = (toLongitude - fromLongitude) * Math.PI / 180

  const a =
    Math.sin(deltaLatitude / 2) * Math.sin(deltaLatitude / 2) +
    Math.cos(fromLatitudeRadians) *
      Math.cos(toLatitudeRadians) *
      Math.sin(deltaLongitude / 2) *
      Math.sin(deltaLongitude / 2)
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))

  return R * c
}
