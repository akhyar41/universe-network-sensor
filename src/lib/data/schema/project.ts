import { z } from "zod/v4"

export const ProjectSchema = z.object({
  broker_url: z.string().default("wss://broker.emqx.io:8084/mqtt"),
  id: z.string().default("1003250005"),
})

export type Project = z.infer<typeof ProjectSchema>
export const ProjectEmpty = ProjectSchema.parse({})