/* eslint-disable @typescript-eslint/no-unsafe-member-access */
/* eslint-disable @typescript-eslint/no-explicit-any */
import '@testing-library/jest-dom/vitest'

// jsdom does not implement ResizeObserver — Radix UI primitives (Slider, Dialog,
// Select, etc.) call it on mount. Stub it for the test environment.
class ResizeObserverStub {
  observe(): void {}
  unobserve(): void {}
  disconnect(): void {}
}

if (typeof globalThis.ResizeObserver === 'undefined') {
  ;(globalThis as unknown as { ResizeObserver: typeof ResizeObserverStub }).ResizeObserver =
    ResizeObserverStub
}

// jsdom does not implement Element.hasPointerCapture / setPointerCapture which
// Radix Select uses on its trigger. Stub them.
if (typeof window !== 'undefined' && !(Element.prototype as any).hasPointerCapture) {
  ;(Element.prototype as any).hasPointerCapture = () => false
  ;(Element.prototype as any).setPointerCapture = () => {}
  ;(Element.prototype as any).releasePointerCapture = () => {}
}

// jsdom does not implement scrollIntoView (Radix Select uses it).
if (typeof window !== 'undefined' && !(Element.prototype as any).scrollIntoView) {
  ;(Element.prototype as any).scrollIntoView = () => {}
}
