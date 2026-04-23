import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Build into `public/` so the JUCE WebView resource provider (plugin/ui/public) serves this UI.
// Source static files live in `static/` (Vite's publicDir) so they are not clobbered by the build.
export default defineConfig({
  plugins: [react()],
  publicDir: 'static',
  build: {
    outDir: 'public',
    emptyOutDir: true,
  },
})
