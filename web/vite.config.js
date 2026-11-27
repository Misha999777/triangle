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
        icons: [
          { src: 'icon.svg', sizes: 'any' },
          { src: 'maskable.svg', sizes: 'any', purpose: 'maskable' },
        ],
      },
    }),
  ],
})
