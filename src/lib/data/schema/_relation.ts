import { z } from "zod/v4"

export const RelationSchema = z.object({
  id: z.string().default(""),
  name: z.string().default(""),
  order: z.number().default(0),
})

export type Relation = z.infer<typeof RelationSchema>
export const RelationEmpty = RelationSchema.parse({})
