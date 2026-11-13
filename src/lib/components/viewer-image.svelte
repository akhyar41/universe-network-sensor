<script lang="ts">
  import { Button } from "#lib/components/ui/button"
  import { Sheet, SheetContent, SheetDescription, SheetHeader, SheetTitle } from "#lib/components/ui/sheet"
  import { useDialog } from "#lib/hooks/use-dialog.svelte"
  import DownloadIcon from "@lucide/svelte/icons/download"
  import XIcon from "@lucide/svelte/icons/x"
  import ZoomInIcon from "@lucide/svelte/icons/zoom-in"
  import ZoomOutIcon from "@lucide/svelte/icons/zoom-out"

  const dialog = useDialog()
  let url = $state("")

  let containerEl = $state<HTMLDivElement | null>(null)
  let imgEl = $state<HTMLImageElement | null>(null)

  // Reactive transform state (drives CSS)
  let scale = $state(1)
  let tx = $state(0)
  let ty = $state(0)
  let isDraggable = $state(false)
  let animated = $state(false)

  // Non-reactive refs — mutated directly for 60fps paths (pinch, drag)
  let scaleRef = 1
  let baseSize = { width: 0, height: 0 }
  let pinchDist: number | null = null
  let pinchScaleStart = 1

  // Single-finger drag tracking
  let dragging = false
  let dragOriginX = 0
  let dragOriginY = 0
  let dragOriginTx = 0
  let dragOriginTy = 0

  const cssTransform = $derived(`translate(${tx}px,${ty}px) scale(${scale})`)
  const cssTransition = $derived(animated ? "transform 0.2s ease-out" : "none")

  function clamp(v: number, lo: number, hi: number) {
    return Math.max(lo, Math.min(hi, v))
  }

  function measureBase() {
    if (!imgEl || !containerEl) return
    const { width, height } = containerEl.getBoundingClientRect()
    const { naturalWidth: nw, naturalHeight: nh } = imgEl
    if (!nw || !nh) return
    const fit = Math.min(width / nw, height / nh, 1)
    baseSize = { width: nw * fit, height: nh * fit }
  }

  function getBounds(s: number) {
    if (!containerEl) return { l: 0, r: 0, t: 0, b: 0 }
    const { width, height } = containerEl.getBoundingClientRect()
    const mx = Math.max(0, (baseSize.width * s - width) / 2)
    const my = Math.max(0, (baseSize.height * s - height) / 2)
    return { l: -mx, r: mx, t: -my, b: my }
  }

  function applyTransform(s: number, x: number, y: number, anim: boolean) {
    const { l, r, t, b } = getBounds(s)
    scaleRef = s
    scale = s
    tx = clamp(x, l, r)
    ty = clamp(y, t, b)
    isDraggable = s > 1
    animated = anim
  }

  function reset() {
    scaleRef = 1
    baseSize = { width: 0, height: 0 }
    pinchDist = null
    dragging = false
    scale = 1
    tx = 0
    ty = 0
    isDraggable = false
    animated = false
  }

  // Button zoom
  function zoomIn() {
    measureBase()
    applyTransform(Math.min(scaleRef + 0.5, 5), tx, ty, true)
  }

  function zoomOut() {
    measureBase()
    const s = Math.max(scaleRef - 0.5, 1)
    applyTransform(s, s === 1 ? 0 : tx, s === 1 ? 0 : ty, true)
  }

  // Touch: pinch + single-finger drag
  function onTouchStart(e: TouchEvent) {
    if (e.touches.length === 2) {
      dragging = false
      measureBase()
      pinchDist = Math.hypot(
        e.touches[0].pageX - e.touches[1].pageX,
        e.touches[0].pageY - e.touches[1].pageY
      )
      pinchScaleStart = scaleRef
    } else if (e.touches.length === 1 && isDraggable) {
      dragging = true
      dragOriginX = e.touches[0].clientX
      dragOriginY = e.touches[0].clientY
      dragOriginTx = tx
      dragOriginTy = ty
      animated = false
    }
  }

  function onTouchMove(e: TouchEvent) {
    if (e.touches.length === 2 && pinchDist !== null) {
      const dist = Math.hypot(
        e.touches[0].pageX - e.touches[1].pageX,
        e.touches[0].pageY - e.touches[1].pageY
      )
      const raw = pinchScaleStart * (dist / pinchDist)

      // Soft resistance beyond min/max
      const s = raw < 1 ? 1 - (1 - raw) * 0.15
        : raw > 5 ? 5 + (raw - 5) * 0.1
          : raw

      const { l, r, t, b } = getBounds(s)
      scaleRef = s
      scale = s
      animated = false
      tx = clamp(tx, l, r)
      ty = clamp(ty, t, b)
    } else if (e.touches.length === 1 && dragging) {
      const { l, r, t, b } = getBounds(scaleRef)
      tx = clamp(dragOriginTx + (e.touches[0].clientX - dragOriginX), l, r)
      ty = clamp(dragOriginTy + (e.touches[0].clientY - dragOriginY), t, b)
    }
  }

  function onTouchEnd(e: TouchEvent) {
    if (pinchDist !== null && e.touches.length < 2) {
      pinchDist = null
      const s = clamp(scaleRef, 1, 5)
      applyTransform(s, s === 1 ? 0 : tx, s === 1 ? 0 : ty, true)
    }
    if (dragging && e.touches.length === 0) {
      dragging = false
      animated = true
    }
  }

  // Mouse/stylus drag via pointer events
  function onPointerDown(e: PointerEvent) {
    if (!isDraggable || !e.isPrimary || pinchDist !== null) return
    dragging = true
    dragOriginX = e.clientX
    dragOriginY = e.clientY
    dragOriginTx = tx
    dragOriginTy = ty
    animated = false
    const target = e.currentTarget as HTMLElement
    target.setPointerCapture(e.pointerId)
  }

  function onPointerMove(e: PointerEvent) {
    if (!dragging || !e.isPrimary) return
    const { l, r, t, b } = getBounds(scaleRef)
    tx = clamp(dragOriginTx + (e.clientX - dragOriginX), l, r)
    ty = clamp(dragOriginTy + (e.clientY - dragOriginY), t, b)
  }

  function onPointerUp(e: PointerEvent) {
    if (!dragging || !e.isPrimary) return
    dragging = false
    animated = true
  }

  $effect(() => {

    function handleView(e: CustomEvent) {
      url = e.detail as string
      reset()
      dialog.setOpen(true)
    }

    window.addEventListener("view-image", handleView as EventListener)
    return () => {
      window.removeEventListener("view-image", handleView as EventListener)
    }
  })

</script>
<Sheet
  open={dialog.state}
  onOpenChange={dialog.setOpen}
  onOpenChangeComplete={() => ""}>
  <SheetContent
    class="h-full! w-full! z-60 p-0 bg-black/80 border-none flex flex-col data-[state=open]:duration-0"
    side="bottom"
    showCloseButton={false}>
    <SheetHeader class="hidden">
      <SheetTitle>View Image</SheetTitle>
      <SheetDescription></SheetDescription>
    </SheetHeader>
    <nav class="flex z-10 bg-black/50 backdrop-blur-sm">
      <div class="flex h-12 items-center px-3 space-x-2">
        <Button variant="ghost"
                size="icon"
                class="rounded-full text-white hover:bg-white/20 hover:text-white"
                aria-label="Close viewer"
                title="Close viewer"
                onclick={() => dialog.setOpen(false)}>
          <XIcon aria-hidden="true"/>
        </Button>
        <Button variant="ghost"
                size="icon"
                class="rounded-full text-white hover:bg-white/20 hover:text-white"
                aria-label="Zoom in"
                title="Zoom in"
                onclick={zoomIn}>
          <ZoomInIcon aria-hidden="true"/>
        </Button>
        <Button variant="ghost"
                size="icon"
                class="rounded-full text-white hover:bg-white/20 hover:text-white"
                aria-label="Zoom out"
                title="Zoom out"
                onclick={zoomOut}>
          <ZoomOutIcon aria-hidden="true"/>
        </Button>
        <Button variant="ghost"
                size="icon"
                class="rounded-full text-white hover:bg-white/20 hover:text-white"
                aria-label="Download image"
                title="Download image"
                onclick={() => url.fileDownload()}>
          <DownloadIcon aria-hidden="true"/>
        </Button>
        <div class="flex-1"></div>
      </div>
    </nav>
    <div bind:this={containerEl}
         role="presentation"
         class="relative flex-1 overflow-hidden touch-none flex items-center justify-center select-none"
         ontouchstart={onTouchStart}
         ontouchmove={onTouchMove}
         ontouchend={onTouchEnd}
         onpointerdown={onPointerDown}
         onpointermove={onPointerMove}
         onpointerup={onPointerUp}
         onpointercancel={onPointerUp}>
      <div style="transform:{cssTransform}; transition:{cssTransition}; will-change:transform"
           class="w-full h-full flex items-center justify-center {isDraggable ? 'cursor-grab active:cursor-grabbing' : ''}">
        <img bind:this={imgEl}
             src={url}
             alt="viewer"
             onload={measureBase}
             draggable="false"
             class="max-h-full max-w-full object-contain pointer-events-none"/>
      </div>
    </div>
  </SheetContent>
</Sheet>
