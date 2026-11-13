import { sveltekit } from "@sveltejs/kit/vite"
import adapter from "@sveltejs/adapter-static"
import tailwindcss from "@tailwindcss/vite"
import { mkdir, readdir, rm } from "node:fs/promises"
import path from "node:path"
import { defineConfig } from "vite"

// https://vite.dev/config/
export default defineConfig(({ command, mode }) => {

  const outDir = `dist`
  const assetsDir = `${outDir}/build`

  const srcDir = path.resolve(import.meta.dirname, "src")
  const generatedDir = path.resolve(import.meta.dirname, ".svelte-kit/generated")

  return {
    envPrefix: "APP_",
    plugins: [
      tailwindcss(),
      sveltekit({
        compilerOptions: {
          // Force runes mode for the project, except for libraries. Can be removed in svelte 6.
          runes: ({filename}) => (filename.split(/[/\\]/).includes("node_modules") ? undefined : true),
        },
        adapter: adapter({
          fallback: "index.html",
          assets: "./dist",
          pages: "./dist",
        }),
        files: {
          assets: "public",
        },
        appDir: "build",
        output: {
          bundleStrategy: "split",
        },
      }),
      {
        name: "cleansing-build",
        buildStart: async function () {
          if (command != "build") return

          await rm(assetsDir, { recursive: true, force: true })
          await mkdir(assetsDir, { recursive: true })
        }
      }
    ],
    optimizeDeps: {
      // Pre-bundle heavy deps at startup so Vite never stops to re-optimize
      // (and force a full page reload) the first time a route imports one.
      include: [
        "jodit", "leaflet", "layerchart", "papaparse", "lodash",
        "axios", "js-cookie", "clsx", "ulid",
        "@tanstack/svelte-query", "@tanstack/svelte-form", "@tanstack/table-core",
        "embla-carousel-svelte", "@lucide/svelte",
      ],
    },
    server: {
      watch: {
        ignored: (filePath: string) => {
          // Normalize to lowercase forward-slash absolute paths so Windows
          // drive-letter casing / separator differences can't wrongly ignore
          // src files (which would break HMR entirely).
          const norm = (p: string) => path.resolve(p).replace(/\\/g, "/").toLowerCase()
          const file = norm(filePath)
          // Watch: these dirs themselves, anything inside them, and their
          // ancestor dirs (so the watcher can descend into them). Ignore
          // everything else. The generated dir must stay watched or Vite keeps
          // serving a stale route manifest whenever a route file is added, and
          // the browser 404s on the new route.
          return !([norm(srcDir), norm(generatedDir)].some(
            dir => file === dir || file.startsWith(dir + "/") || dir.startsWith(file + "/")
          ))
        },
      }
    },
  }
})
