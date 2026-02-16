import { useEffect } from 'react';
import { motion } from 'framer-motion';

interface Props {
  onReady: () => void;
  isLoaded: boolean;
}

export default function LoadingOverlay({ onReady, isLoaded }: Props) {
  useEffect(() => {
    if (isLoaded) {
      const timer = setTimeout(onReady, 600);
      return () => clearTimeout(timer);
    }
  }, [isLoaded, onReady]);

  return (
    <motion.div
      className="loading-overlay"
      initial={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.3 }}
    >
      <motion.div
        className="loading-content"
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ type: 'spring', stiffness: 200, damping: 25 }}
      >
        <div className="loading-icon">{'\uD83D\uDD22'}</div>
        <h1>CEmu Web</h1>
        <p>TI-84 Plus CE Emulator</p>
        <motion.div
          className="loading-bar"
          initial={{ scaleX: 0 }}
          animate={{ scaleX: isLoaded ? 1 : 0.6 }}
          transition={{ duration: isLoaded ? 0.3 : 2, ease: 'easeInOut' }}
        />
      </motion.div>

      <style>{`
        .loading-overlay {
          position: fixed;
          inset: 0;
          display: flex;
          align-items: center;
          justify-content: center;
          background: #121214;
          z-index: 100;
        }
        .loading-content {
          text-align: center;
        }
        .loading-icon {
          font-size: 4rem;
          margin-bottom: 1rem;
        }
        .loading-content h1 {
          font-size: 1.5rem;
          margin-bottom: 0.25rem;
        }
        .loading-content p {
          color: var(--text-secondary);
          font-size: 0.875rem;
          margin-bottom: 1.5rem;
        }
        .loading-bar {
          width: 200px;
          height: 3px;
          background: var(--btn-2nd);
          border-radius: 2px;
          transform-origin: left;
          margin: 0 auto;
        }
      `}</style>
    </motion.div>
  );
}
