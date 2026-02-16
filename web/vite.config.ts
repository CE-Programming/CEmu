import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';
import fs from 'fs';

// Copy shared assets from gui/qt/resources/ into web/public/ so Vite can
// serve them.  The copies are gitignored — the Qt resources are canonical.
function copySharedAssets() {
  const root = path.resolve(__dirname, '..');
  const copies: [string, string][] = [
    [path.join(root, 'gui/qt/resources/skin'),         path.join(__dirname, 'public/skin')],
    [path.join(root, 'gui/qt/resources/custom_fonts'),  path.join(__dirname, 'public/fonts')],
  ];
  return {
    name: 'copy-shared-assets',
    buildStart() {
      for (const [src, dest] of copies) {
        fs.mkdirSync(dest, { recursive: true });
        for (const file of fs.readdirSync(src)) {
          const srcFile = path.join(src, file);
          const destFile = path.join(dest, file);
          if (!fs.existsSync(destFile) ||
              fs.statSync(srcFile).mtimeMs > fs.statSync(destFile).mtimeMs) {
            fs.copyFileSync(srcFile, destFile);
          }
        }
      }
    },
  };
}

export default defineConfig({
  plugins: [copySharedAssets(), react()],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  server: {
    allowedHosts: process.env.VITE_ALLOWED_HOSTS?.split(',') ?? [],
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
    },
  },
  assetsInclude: ['**/*.wasm'],
});
