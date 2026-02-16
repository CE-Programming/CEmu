import { useState, useCallback, useEffect } from 'react';
import type { EmulatorBridge } from '../wasm/EmulatorBridge';

export function useFileTransfer(bridge: EmulatorBridge | null) {
  const [transferring, setTransferring] = useState(false);
  const [progress, setProgress] = useState<{ value: number; total: number } | null>(null);

  useEffect(() => {
    (window as any).transferProgressCallback = (value: number, total: number) => {
      setProgress({ value, total });
      if (value >= total) {
        setTransferring(false);
        setProgress(null);
      }
    };
    (window as any).transferErrorCallback = () => {
      setTransferring(false);
      setProgress(null);
      console.error('File transfer failed');
    };

    return () => {
      delete (window as any).transferProgressCallback;
      delete (window as any).transferErrorCallback;
    };
  }, []);

  const sendFile = useCallback(async (file: File) => {
    if (!bridge) return;
    setTransferring(true);
    setProgress(null);

    const buffer = await file.arrayBuffer();
    const data = new Uint8Array(buffer);
    bridge.sendFile(file.name, data);
  }, [bridge]);

  return { sendFile, transferring, progress };
}
