import { useEffect, useRef } from 'react';
import { KEYBOARD_MAP } from '../utils/keyboardMap';
import type { EmulatorBridge } from '../wasm/EmulatorBridge';

export function useKeyboard(bridge: EmulatorBridge | null) {
  const pressedKeys = useRef<Set<string>>(new Set());

  useEffect(() => {
    if (!bridge) return;

    function handleKeyDown(e: KeyboardEvent) {
      if (e.repeat) return;
      const mapping = KEYBOARD_MAP[e.code];
      if (!mapping) return;

      e.preventDefault();
      if (pressedKeys.current.has(e.code)) return;
      pressedKeys.current.add(e.code);
      bridge!.pressKey(mapping.row, mapping.col);
    }

    function handleKeyUp(e: KeyboardEvent) {
      const mapping = KEYBOARD_MAP[e.code];
      if (!mapping) return;

      e.preventDefault();
      pressedKeys.current.delete(e.code);
      bridge!.releaseKey(mapping.row, mapping.col);
    }

    function handleBlur() {
      for (const code of pressedKeys.current) {
        const mapping = KEYBOARD_MAP[code];
        if (mapping) {
          bridge!.releaseKey(mapping.row, mapping.col);
        }
      }
      pressedKeys.current.clear();
    }

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    window.addEventListener('blur', handleBlur);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
      window.removeEventListener('blur', handleBlur);
      handleBlur();
    };
  }, [bridge]);
}
