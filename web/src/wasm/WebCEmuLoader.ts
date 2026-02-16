import type { WebCEmuModule, WebCEmuFactory } from '../types/wasm';

let modulePromise: Promise<WebCEmuModule> | null = null;

export async function loadWebCEmu(): Promise<WebCEmuModule> {
  if (modulePromise) return modulePromise;

  modulePromise = (async () => {
    // Load the Emscripten module via fetch + blob URL to avoid Vite's
    // public directory import restriction in dev mode
    const response = await fetch('/wasm/WebCEmu.js');
    if (!response.ok) {
      throw new Error(`Failed to fetch WebCEmu.js: ${response.status}`);
    }
    const source = await response.text();
    const blob = new Blob([source], { type: 'application/javascript' });
    const blobUrl = URL.createObjectURL(blob);
    const wasmModule = await import(/* @vite-ignore */ blobUrl);
    URL.revokeObjectURL(blobUrl);
    const factory: WebCEmuFactory = wasmModule.default;

    const module = await factory({
      locateFile(path: string) {
        if (path.endsWith('.wasm')) {
          return '/wasm/WebCEmu.wasm';
        }
        return path;
      },
      print(text: string) {
        console.log('[CEmu]', text);
      },
      printErr(text: string) {
        console.warn('[CEmu]', text);
      },
    });

    return module;
  })();

  return modulePromise;
}
