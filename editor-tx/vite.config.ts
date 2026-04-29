import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { VitePWA } from 'vite-plugin-pwa'
import { resolve } from 'path'

export default defineConfig({
  plugins: [
    react(),
    VitePWA({
      registerType: 'autoUpdate',
      manifest: {
        name: 'T16 Configurator',
        short_name: 'T16',
        description: 'Configure your Topo T16 MIDI controller',
        theme_color: '#6246ea',
        background_color: '#faf9fe',
        display: 'standalone',
        orientation: 'any',
        start_url: '/',
        scope: '/',
        icons: [
          { src: 'icon-192.png', sizes: '192x192', type: 'image/png' },
          { src: 'icon-512.png', sizes: '512x512', type: 'image/png' },
          { src: 'icon-512.png', sizes: '512x512', type: 'image/png', purpose: 'maskable' },
        ],
      },
      workbox: {
        globPatterns: ['**/*.{js,css,html,ico,png,svg,woff2}'],
        cleanupOutdatedCaches: true,
      },
    }),
  ],
  resolve: {
    alias: {
      '@': resolve(__dirname, './src'),
    },
  },
  // Include .bin (already in default Vite list) and .placeholder so the T32
  // firmware placeholder ships in the bundle until Phase 12 hardware bring-up
  // produces a tagged binary (Phase 14-05).
  assetsInclude: ['**/*.bin', '**/*.placeholder'],
  server: {
    watch: { usePolling: true },
  },
})
