import { defineConfig } from 'vite'
import { VitePWA } from 'vite-plugin-pwa'
import react from '@vitejs/plugin-react'

export default defineConfig({
  base: './',
  plugins: [
    react(),
    VitePWA({
      manifest: {
        name: 'Triangle',
        short_name: 'Triangle',
        theme_color: '#212121',
        background_color: '#ffffff',
        icons: [
          {
            src: "icon.svg",
            type: "image/svg+xml",
            sizes: 'any',
            purpose: 'any',
          },
          {
            src: "maskable.svg",
            type: "image/svg+xml",
            sizes: 'any',
            purpose: 'maskable',
          },
        ],
      },
    }),
  ],
})
