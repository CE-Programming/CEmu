import { useState, useCallback } from 'react';
import { AnimatePresence } from 'framer-motion';
import LoadingOverlay from './components/LoadingOverlay/LoadingOverlay';
import RomLoader from './components/RomLoader/RomLoader';
import Calculator from './components/Calculator/Calculator';
import { useEmulator } from './hooks/useEmulator';

type AppState = 'loading' | 'rom-select' | 'running';

export default function App() {
  const [appState, setAppState] = useState<AppState>('loading');
  const { module, bridge, isLoading, error } = useEmulator();

  const handleWasmReady = useCallback(() => {
    setAppState('rom-select');
  }, []);

  const handleRomLoaded = useCallback(() => {
    setAppState('running');
  }, []);

  if (isLoading || appState === 'loading') {
    return <LoadingOverlay onReady={handleWasmReady} isLoaded={!isLoading} />;
  }

  return (
    <AnimatePresence mode="wait">
      {appState === 'rom-select' && bridge ? (
        <RomLoader key="rom-loader" bridge={bridge} onRomLoaded={handleRomLoaded} />
      ) : appState === 'running' && bridge && module ? (
        <Calculator key="calculator" bridge={bridge} module={module} />
      ) : error ? (
        <div className="error-screen">
          <h2>Failed to load emulator</h2>
          <p>{error}</p>
        </div>
      ) : null}
    </AnimatePresence>
  );
}
