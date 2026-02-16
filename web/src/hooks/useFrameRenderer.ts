import { useEffect, useRef } from 'react';
import { FrameRenderer } from '../wasm/FrameRenderer';
import type { EmulatorBridge } from '../wasm/EmulatorBridge';
import type { WebCEmuModule } from '../types/wasm';

export function useFrameRenderer(
  canvasRef: React.RefObject<HTMLCanvasElement | null>,
  module: WebCEmuModule | null,
  bridge: EmulatorBridge | null
) {
  const rendererRef = useRef<FrameRenderer | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !module || !bridge || !bridge.booted) return;

    const renderer = new FrameRenderer(canvas, module, bridge);
    rendererRef.current = renderer;
    renderer.start();

    return () => {
      renderer.stop();
      rendererRef.current = null;
    };
  }, [canvasRef, module, bridge, bridge?.booted]);

  return rendererRef;
}
