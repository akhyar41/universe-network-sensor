export const APP_CODE = import.meta.env.APP_CODE
export const APP_NAME = import.meta.env.APP_NAME
export const APP_COMP = import.meta.env.APP_COMP
export const APP_MODE = import.meta.env.MODE
export const APP_VERSION = import.meta.env.APP_VERSION

export const APP_URL = import.meta.env.APP_GUI_URL
export const API_URL = import.meta.env.APP_URL

export const MQTT_HOST = import.meta.env.APP_MQTT_HOST
export const MQTT_USERNAME = import.meta.env.APP_MQTT_USER
export const MQTT_PASSWORD = import.meta.env.APP_MQTT_PASS
export const MQTT_TOPIC_PREFIXES = import.meta.env.APP_MQTT_TOPIC_PREFIXES
  .split(",")
  .map((prefix: string) => prefix.trim())
  .filter(Boolean)
