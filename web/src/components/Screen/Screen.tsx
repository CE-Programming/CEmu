import { useRef } from 'react';
import { motion } from 'framer-motion';
import { useFrameRenderer } from '../../hooks/useFrameRenderer';
import { getSkinImage, SKIN_W, SKIN_H, LCD_X, LCD_Y, LCD_W, LCD_H } from '../../config/skinEngine';
import type { DeviceType } from '../../config/skinEngine';
import type { EmulatorBridge } from '../../wasm/EmulatorBridge';
import type { WebCEmuModule } from '../../types/wasm';

interface Props {
  bridge: EmulatorBridge;
  module: WebCEmuModule;
  device: DeviceType;
  python: boolean;
}

const lcdLeft   = `${(LCD_X / SKIN_W * 100).toFixed(4)}%`;
const lcdTop    = `${(LCD_Y / SKIN_H * 100).toFixed(4)}%`;
const lcdWidth  = `${(LCD_W / SKIN_W * 100).toFixed(4)}%`;
const lcdHeight = `${(LCD_H / SKIN_H * 100).toFixed(4)}%`;
const aspectPad = `${(SKIN_H / SKIN_W * 100).toFixed(4)}%`;

export default function Screen({ bridge, module, device, python }: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  useFrameRenderer(canvasRef, module, bridge);

  const skinUrl = getSkinImage(device, python);

  return (
    <motion.div
      initial={{ opacity: 0.8 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.6 }}
      style={{
        position: 'relative',
        width: '100%',
        paddingBottom: aspectPad,
        backgroundImage: `url(${skinUrl})`,
        backgroundSize: '100% 100%',
        backgroundRepeat: 'no-repeat',
      }}
    >
      <motion.canvas
        ref={canvasRef}
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ duration: 0.8, delay: 0.3 }}
        style={{
          position: 'absolute',
          left: lcdLeft,
          top: lcdTop,
          width: lcdWidth,
          height: lcdHeight,
          imageRendering: 'pixelated',
          background: '#1a1a1e',
        }}
      />
    </motion.div>
  );
}
