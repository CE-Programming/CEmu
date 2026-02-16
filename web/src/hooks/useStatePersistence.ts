import { useCallback } from 'react';
import type { EmulatorBridge } from '../wasm/EmulatorBridge';
import { setItem, getItem } from '../utils/indexedDB';

const STATE_STORAGE_KEY = 'cemu-state';

export function useStatePersistence(bridge: EmulatorBridge | null) {
  const saveState = useCallback(async () => {
    if (!bridge) return false;
    const data = bridge.saveState();
    if (!data) return false;
    await setItem(STATE_STORAGE_KEY, data);
    return true;
  }, [bridge]);

  const loadState = useCallback(async () => {
    if (!bridge) return false;
    const data = await getItem<Uint8Array>(STATE_STORAGE_KEY);
    if (!data) return false;
    return bridge.loadState(data);
  }, [bridge]);

  return { saveState, loadState };
}
