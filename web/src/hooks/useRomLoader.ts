import { useState, useCallback } from 'react';
import type { EmulatorBridge } from '../wasm/EmulatorBridge';
import { setItem, getItem } from '../utils/indexedDB';

const ROM_STORAGE_KEY = 'cemu-rom';

export function useRomLoader(bridge: EmulatorBridge | null) {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const loadRomFromFile = useCallback(async (file: File) => {
    if (!bridge) return false;
    setLoading(true);
    setError(null);

    try {
      const buffer = await file.arrayBuffer();
      const data = new Uint8Array(buffer);
      bridge.loadRom(data);
      await setItem(ROM_STORAGE_KEY, data);
      setLoading(false);
      return true;
    } catch (err) {
      const msg = err instanceof Error ? err.message : 'Failed to load ROM';
      setError(msg);
      setLoading(false);
      return false;
    }
  }, [bridge]);

  const loadRomFromStorage = useCallback(async () => {
    if (!bridge) return false;
    setLoading(true);

    try {
      const data = await getItem<Uint8Array>(ROM_STORAGE_KEY);
      if (!data) {
        setLoading(false);
        return false;
      }
      bridge.loadRom(data);
      setLoading(false);
      return true;
    } catch {
      setLoading(false);
      return false;
    }
  }, [bridge]);

  return { loadRomFromFile, loadRomFromStorage, loading, error };
}
