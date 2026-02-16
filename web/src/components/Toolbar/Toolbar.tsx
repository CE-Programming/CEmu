import { useState, useCallback } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import type { ColorScheme } from '../../config/skinEngine';

interface Props {
  onReset: () => void;
  onSave: () => Promise<boolean>;
  onLoad: () => Promise<boolean>;
  schemeId: string;
  schemes: ColorScheme[];
  onSchemeChange: (id: string) => void;
}

export default function Toolbar({ onReset, onSave, onLoad, schemeId, schemes, onSchemeChange }: Props) {
  const [status, setStatus] = useState<string | null>(null);

  const handleSave = useCallback(async () => {
    const ok = await onSave();
    setStatus(ok ? 'Saved' : 'Failed');
    setTimeout(() => setStatus(null), 1500);
  }, [onSave]);

  const handleLoad = useCallback(async () => {
    const ok = await onLoad();
    setStatus(ok ? 'Loaded' : 'No save');
    setTimeout(() => setStatus(null), 1500);
  }, [onLoad]);

  return (
    <div className="toolbar">
      <select
        className="tb-scheme"
        value={schemeId}
        onChange={(e) => onSchemeChange(e.target.value)}
        title="Color Scheme"
      >
        {schemes.map(s => (
          <option key={s.id} value={s.id}>{s.name}</option>
        ))}
      </select>

      <div className="tb-actions">
        <motion.button className="tb-btn" whileTap={{ scale: 0.9 }} onClick={onReset} title="Reset">
          <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
            <path d="M2 6a4 4 0 1 1 1.17 2.83" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
            <path d="M2 9V6h3" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" strokeLinejoin="round" />
          </svg>
        </motion.button>
        <motion.button className="tb-btn" whileTap={{ scale: 0.9 }} onClick={handleSave} title="Save State">
          <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
            <path d="M6 2v6M3.5 5.5L6 8l2.5-2.5" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" strokeLinejoin="round" />
            <path d="M2 9.5h8" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
          </svg>
        </motion.button>
        <motion.button className="tb-btn" whileTap={{ scale: 0.9 }} onClick={handleLoad} title="Load State">
          <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
            <path d="M6 8V2M3.5 4.5L6 2l2.5 2.5" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" strokeLinejoin="round" />
            <path d="M2 9.5h8" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
          </svg>
        </motion.button>

        <AnimatePresence>
          {status && (
            <motion.span
              className="tb-status"
              initial={{ opacity: 0, x: -4 }}
              animate={{ opacity: 1, x: 0 }}
              exit={{ opacity: 0 }}
            >
              {status}
            </motion.span>
          )}
        </AnimatePresence>
      </div>

      <style>{`
        .toolbar {
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 6px 8px;
          gap: 6px;
        }
        .tb-scheme {
          background: rgba(255, 255, 255, 0.08);
          color: #ccc;
          border: 1px solid rgba(255, 255, 255, 0.1);
          border-radius: 4px;
          font-size: 11px;
          font-family: inherit;
          padding: 3px 6px;
          cursor: pointer;
          outline: none;
          max-width: 130px;
        }
        .tb-scheme:focus {
          border-color: rgba(255, 255, 255, 0.25);
        }
        .tb-actions {
          display: flex;
          align-items: center;
          gap: 4px;
        }
        .tb-btn {
          background: rgba(255, 255, 255, 0.06);
          color: #888890;
          border: 1px solid rgba(255, 255, 255, 0.06);
          width: 22px;
          height: 22px;
          border-radius: 50%;
          cursor: pointer;
          display: flex;
          align-items: center;
          justify-content: center;
          touch-action: manipulation;
          user-select: none;
          padding: 0;
        }
        .tb-btn:hover {
          background: rgba(255, 255, 255, 0.1);
          color: #fff;
        }
        .tb-status {
          font-size: 9px;
          color: #f5a623;
          margin-left: 4px;
        }
      `}</style>
    </div>
  );
}
