import { useState, useEffect, useRef } from 'react';
import { loadWebCEmu } from '../wasm/WebCEmuLoader';
import { EmulatorBridge } from '../wasm/EmulatorBridge';
import type { WebCEmuModule } from '../types/wasm';

export function useEmulator() {
  const [module, setModule] = useState<WebCEmuModule | null>(null);
  const [bridge, setBridge] = useState<EmulatorBridge | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const initialized = useRef(false);

  useEffect(() => {
    if (initialized.current) return;
    initialized.current = true;

    loadWebCEmu()
      .then((mod) => {
        const b = new EmulatorBridge(mod);
        setModule(mod);
        setBridge(b);
        setIsLoading(false);
      })
      .catch((err) => {
        console.error('Failed to load WASM module:', err);
        setError(err instanceof Error ? err.message : String(err));
        setIsLoading(false);
      });
  }, []);

  return { module, bridge, isLoading, error };
}
