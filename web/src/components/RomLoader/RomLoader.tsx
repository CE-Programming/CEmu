import { useState, useCallback, useEffect, useRef } from 'react';
import { motion } from 'framer-motion';
import type { EmulatorBridge } from '../../wasm/EmulatorBridge';
import { useRomLoader } from '../../hooks/useRomLoader';

interface Props {
  bridge: EmulatorBridge;
  onRomLoaded: () => void;
}

export default function RomLoader({ bridge, onRomLoaded }: Props) {
  const { loadRomFromFile, loadRomFromStorage, loading, error } = useRomLoader(bridge);
  const [isDragging, setIsDragging] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const triedStorage = useRef(false);

  useEffect(() => {
    if (triedStorage.current) return;
    triedStorage.current = true;
    loadRomFromStorage().then((loaded) => {
      if (loaded) onRomLoaded();
    });
  }, [loadRomFromStorage, onRomLoaded]);

  const handleFile = useCallback(async (file: File) => {
    const success = await loadRomFromFile(file);
    if (success) onRomLoaded();
  }, [loadRomFromFile, onRomLoaded]);

  const handleDrop = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(false);
    const file = e.dataTransfer.files[0];
    if (file) handleFile(file);
  }, [handleFile]);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    setIsDragging(true);
  }, []);

  const handleDragLeave = useCallback(() => {
    setIsDragging(false);
  }, []);

  const handleClick = useCallback(() => {
    fileInputRef.current?.click();
  }, []);

  const handleInputChange = useCallback((e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) handleFile(file);
  }, [handleFile]);

  return (
    <motion.div
      className="rom-loader"
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -20 }}
      transition={{ type: 'spring', stiffness: 200, damping: 25 }}
    >
      <div
        className={`drop-zone${isDragging ? ' drop-zone--active' : ''}`}
        onDrop={handleDrop}
        onDragOver={handleDragOver}
        onDragLeave={handleDragLeave}
        onClick={handleClick}
      >
        <div className="drop-zone-icon">{'\uD83D\uDCC1'}</div>
        <h2>Load Calculator ROM</h2>
        <p>Drag & drop a TI-84 Plus CE ROM file here, or click to browse</p>
        {loading && <p className="drop-zone-status">Loading ROM...</p>}
        {error && <p className="drop-zone-error">{error}</p>}
        <input
          ref={fileInputRef}
          type="file"
          accept=".rom,.ROM"
          onChange={handleInputChange}
          style={{ display: 'none' }}
        />
      </div>
      <p className="rom-notice">
        You must provide your own ROM file. ROM files are copyrighted by Texas Instruments.
      </p>

      <style>{`
        .rom-loader {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          padding: 2rem;
          gap: 1.5rem;
          min-height: 400px;
        }
        .drop-zone {
          border: 2px dashed var(--drop-zone-border);
          border-radius: 16px;
          padding: 3rem 2rem;
          text-align: center;
          cursor: pointer;
          transition: background 0.2s, border-color 0.2s;
          background: var(--drop-zone-bg);
          max-width: 400px;
          width: 100%;
        }
        .drop-zone:hover,
        .drop-zone--active {
          background: rgba(90, 90, 255, 0.15);
          border-color: #7a7aff;
        }
        .drop-zone-icon {
          font-size: 3rem;
          margin-bottom: 1rem;
        }
        .drop-zone h2 {
          margin-bottom: 0.5rem;
          font-size: 1.25rem;
        }
        .drop-zone p {
          color: var(--text-secondary);
          font-size: 0.875rem;
        }
        .drop-zone-status {
          color: var(--btn-2nd) !important;
          margin-top: 0.5rem;
        }
        .drop-zone-error {
          color: #ef5350 !important;
          margin-top: 0.5rem;
        }
        .rom-notice {
          font-size: 0.75rem;
          color: var(--text-secondary);
          text-align: center;
          max-width: 360px;
          opacity: 0.7;
        }
      `}</style>
    </motion.div>
  );
}
