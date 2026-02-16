import { useRef, useState, useEffect, useCallback } from 'react';
import { KEYS, BASE_W, BASE_H } from './keypadLayout';
import KeypadButton from './KeypadButton';
import DPad from './DPad';
import { getKeyColors } from '../../config/skinEngine';
import type { ColorScheme, DeviceConfig } from '../../config/skinEngine';
import type { EmulatorBridge } from '../../wasm/EmulatorBridge';

interface Props {
  bridge: EmulatorBridge;
  scheme: ColorScheme;
  device: DeviceConfig;
}

export default function Keypad({ bridge, scheme, device }: Props) {
  const wrapperRef = useRef<HTMLDivElement>(null);
  const [scale, setScale] = useState(1);

  useEffect(() => {
    const el = wrapperRef.current;
    if (!el) return;
    const observer = new ResizeObserver((entries) => {
      const w = entries[0].contentRect.width;
      setScale(w / BASE_W);
    });
    observer.observe(el);
    return () => observer.disconnect();
  }, []);

  const handlePress = useCallback((row: number, col: number) => {
    bridge.pressKey(row, col);
  }, [bridge]);

  const handleRelease = useCallback((row: number, col: number) => {
    bridge.releaseKey(row, col);
  }, [bridge]);

  const gradient = `linear-gradient(to right, ${scheme.sides} 0%, ${scheme.center} 18%, ${scheme.center} 82%, ${scheme.sides} 100%)`;

  return (
    <div
      ref={wrapperRef}
      style={{
        width: '100%',
        height: BASE_H * scale,
        overflow: 'hidden',
      }}
    >
      <div
        style={{
          width: BASE_W,
          height: BASE_H,
          transform: `scale(${scale})`,
          transformOrigin: 'top left',
          position: 'relative',
          background: gradient,
          borderRadius: '0 0 20px 20px',
          fontFamily: "'Liberation Sans Narrow', 'Arial Narrow', sans-serif",
          fontWeight: 700,
        }}
      >
        {KEYS.map((k) => {
          const colors = getKeyColors(k.keyType, scheme, device);
          return (
            <KeypadButton
              key={`${k.row}-${k.col}`}
              {...k}
              fillColor={colors.fill}
              textColor={colors.text}
              secondColor={device.secondColor}
              alphaColor={device.alphaColor}
              onPress={handlePress}
              onRelease={handleRelease}
            />
          );
        })}

        <DPad
          onPress={handlePress}
          onRelease={handleRelease}
          keyColor={scheme.other}
          textColor={scheme.text}
        />
      </div>
    </div>
  );
}
