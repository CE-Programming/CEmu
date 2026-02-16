import { useRef, useState, useCallback } from 'react';
import { motion } from 'framer-motion';
import Screen from '../Screen/Screen';
import Keypad from '../Keypad/Keypad';
import Toolbar from '../Toolbar/Toolbar';
import FileTransfer from '../FileTransfer/FileTransfer';
import { useKeyboard } from '../../hooks/useKeyboard';
import { useStatePersistence } from '../../hooks/useStatePersistence';
import { useFileTransfer } from '../../hooks/useFileTransfer';
import { SCHEMES, getDeviceConfig } from '../../config/skinEngine';
import type { ColorScheme, DeviceType } from '../../config/skinEngine';
import type { EmulatorBridge } from '../../wasm/EmulatorBridge';
import type { WebCEmuModule } from '../../types/wasm';

interface Props {
  bridge: EmulatorBridge;
  module: WebCEmuModule;
}

export default function Calculator({ bridge, module }: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  useKeyboard(bridge);
  const { saveState, loadState } = useStatePersistence(bridge);
  const { sendFile, transferring, progress } = useFileTransfer(bridge);

  const [scheme, setScheme] = useState<ColorScheme>(SCHEMES[0]);
  const [deviceType] = useState<DeviceType>('TI84PCE');
  const [python] = useState(false);
  const device = getDeviceConfig(deviceType, python);

  const handleSchemeChange = useCallback((id: string) => {
    const s = SCHEMES.find(sc => sc.id === id);
    if (s) setScheme(s);
  }, []);

  return (
    <>
      <motion.div
        ref={containerRef}
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        transition={{ type: 'spring', stiffness: 200, damping: 25 }}
        style={{
          width: 'var(--calc-width)',
          maxWidth: 380,
          display: 'flex',
          flexDirection: 'column',
          position: 'relative',
          overflow: 'hidden',
        }}
      >
        <Toolbar
          onReset={() => bridge.reset()}
          onSave={saveState}
          onLoad={loadState}
          schemeId={scheme.id}
          schemes={SCHEMES}
          onSchemeChange={handleSchemeChange}
        />

        <Screen
          bridge={bridge}
          module={module}
          device={deviceType}
          python={python}
        />

        <Keypad
          bridge={bridge}
          scheme={scheme}
          device={device}
        />

        <FileTransfer
          onFileDrop={sendFile}
          transferring={transferring}
          progress={progress}
        />
      </motion.div>

      {/* Landscape blocker for mobile */}
      <div className="landscape-block">
        <div className="landscape-icon">
          <svg width="48" height="48" viewBox="0 0 48 48" fill="none">
            <rect x="12" y="4" width="24" height="40" rx="3" stroke="#666" strokeWidth="2" fill="none" />
            <circle cx="24" cy="40" r="2" fill="#666" />
            <path d="M20 20l4-4 4 4" stroke="#888" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
            <path d="M24 16v12" stroke="#888" strokeWidth="2" strokeLinecap="round" />
          </svg>
        </div>
        <p className="landscape-text">Rotate to portrait mode</p>
      </div>

      <style>{`
        .landscape-block {
          display: none;
          position: fixed;
          inset: 0;
          z-index: 9999;
          background: #0d0d0f;
          align-items: center;
          justify-content: center;
          flex-direction: column;
          gap: 12px;
        }
        .landscape-text {
          color: #888890;
          font-size: 14px;
        }
        .landscape-icon {
          animation: rotate-hint 2.5s ease-in-out infinite;
        }
        @keyframes rotate-hint {
          0%, 100% { transform: rotate(0deg); }
          30% { transform: rotate(-90deg); }
          70% { transform: rotate(-90deg); }
        }
        @media (max-height: 480px) and (orientation: landscape) and (hover: none) {
          .landscape-block { display: flex; }
        }
      `}</style>
    </>
  );
}
