import { MQTT_HOST, MQTT_PASSWORD, MQTT_TOPIC_PREFIXES, MQTT_USERNAME } from "#lib/env"
import { createMqttConnection } from "#lib/mqtt"
import type { MqttStatus } from "#lib/types/device"
import type { MqttClient } from "mqtt"

class MqttConnection {

  status = $state<MqttStatus>("idle")
  error = $state<string | null>(null)
  lastMessageAt = $state<number | null>(null)

  #client: MqttClient | null = null

  start() {
    if (this.#client) return

    if (!MQTT_HOST) {
      this.status = "error"
      this.error = "APP_MQTT_HOST is not configured"
      return
    }

    this.error = null

    this.#client = createMqttConnection(
      {
        url: MQTT_HOST,
        username: MQTT_USERNAME,
        password: MQTT_PASSWORD,
        topicPrefixes: MQTT_TOPIC_PREFIXES,
      },
      {
        onStatus: status => {
          this.status = status
        },
        onError: message => {
          this.error = message
        },
        onMessage: at => {
          this.lastMessageAt = at
          this.error = null
        },
      },
    )
  }

  stop() {
    this.#client?.end(true)
    this.#client = null
    this.status = "idle"
  }
}

export const mqtt = new MqttConnection()
