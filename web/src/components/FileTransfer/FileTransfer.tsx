import { useState, useCallback, useEffect } from 'react';
import { AnimatePresence, motion } from 'framer-motion';

interface Props {
  onFileDrop: (file: File) => void;
  transferring: boolean;
  progress: { value: number; total: number } | null;
}

export default function FileTransfer({ onFileDrop, transferring, progress }: Props) {
  const [isDragging, setIsDragging] = useState(false);

  // Use document-level drag events so we don't need an overlay stealing pointer events
  useEffect(() => {
    let dragCounter = 0;

    const handleDragEnter = (e: DragEvent) => {
      e.preventDefault();
      dragCounter++;
      if (e.dataTransfer?.types.includes('Files')) {
        setIsDragging(true);
      }
    };

    const handleDragLeave = (e: DragEvent) => {
      e.preventDefault();
      dragCounter--;
      if (dragCounter === 0) setIsDragging(false);
    };

    const handleDragOver = (e: DragEvent) => {
      e.preventDefault();
    };

    const handleDrop = (e: DragEvent) => {
      e.preventDefault();
      dragCounter = 0;
      setIsDragging(false);
      const file = e.dataTransfer?.files[0];
      if (file && (file.name.endsWith('.8xp') || file.name.endsWith('.8xv') || file.name.endsWith('.8xg'))) {
        onFileDrop(file);
      }
    };

    document.addEventListener('dragenter', handleDragEnter);
    document.addEventListener('dragleave', handleDragLeave);
    document.addEventListener('dragover', handleDragOver);
    document.addEventListener('drop', handleDrop);
    return () => {
      document.removeEventListener('dragenter', handleDragEnter);
      document.removeEventListener('dragleave', handleDragLeave);
      document.removeEventListener('dragover', handleDragOver);
      document.removeEventListener('drop', handleDrop);
    };
  }, [onFileDrop]);

  return (
    <>
      <AnimatePresence>
        {isDragging && (
          <motion.div
            className="file-transfer-overlay"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
          >
            <p>Drop .8xp file to transfer</p>
          </motion.div>
        )}
      </AnimatePresence>
      <AnimatePresence>
        {transferring && (
          <motion.div
            className="file-transfer-progress"
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: 20 }}
          >
            Transferring{progress ? ` ${Math.round((progress.value / progress.total) * 100)}%` : '...'}
          </motion.div>
        )}
      </AnimatePresence>

      <style>{`
        .file-transfer-overlay {
          position: absolute;
          inset: 0;
          background: rgba(90, 90, 255, 0.2);
          backdrop-filter: blur(4px);
          display: flex;
          align-items: center;
          justify-content: center;
          border-radius: 20px;
          border: 2px dashed var(--drop-zone-border);
          z-index: 10;
          pointer-events: none;
        }
        .file-transfer-overlay p {
          font-size: 1.125rem;
          font-weight: 600;
        }
        .file-transfer-progress {
          position: absolute;
          bottom: 12px;
          left: 50%;
          transform: translateX(-50%);
          background: var(--toolbar-bg);
          padding: 6px 16px;
          border-radius: 20px;
          font-size: 0.8rem;
          z-index: 11;
        }
      `}</style>
    </>
  );
}
