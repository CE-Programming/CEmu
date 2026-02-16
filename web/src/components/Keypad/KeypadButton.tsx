import { useCallback, useState } from 'react';
import { KEY_BORDER_COLOR, KEY_BORDER_WIDTH, PRESSED_OVERLAY } from '../../config/skinEngine';
import type { KeyType } from './keypadLayout';

interface Props {
  label: string;
  row: number;
  col: number;
  x: number;
  y: number;
  w: number;
  h: number;
  keyType: KeyType;
  secondLabel?: string;
  alphaLabel?: string;
  fillColor: string;
  textColor: string;
  secondColor: string;
  alphaColor: string;
  onPress: (row: number, col: number) => void;
  onRelease: (row: number, col: number) => void;
}

function getFontSize(keyType: KeyType, label: string): number {
  switch (keyType) {
    case 'num':    return label.length > 2 ? 6 : 8;
    case 'oper':   return label === 'enter' ? 4.5 : 8;
    case 'graph':  return 4;
    case 'second': return 5;
    case 'alpha':  return 5;
    case 'other':
    default:       return 4.5;
  }
}

export default function KeypadButton({
  label, row, col, x, y, w, h, keyType,
  secondLabel, alphaLabel,
  fillColor, textColor, secondColor, alphaColor,
  onPress, onRelease,
}: Props) {
  const [pressed, setPressed] = useState(false);

  const handleDown = useCallback((e: React.MouseEvent | React.TouchEvent) => {
    e.preventDefault();
    setPressed(true);
    onPress(row, col);
  }, [row, col, onPress]);

  const handleUp = useCallback((e: React.MouseEvent | React.TouchEvent) => {
    e.preventDefault();
    setPressed(false);
    onRelease(row, col);
  }, [row, col, onRelease]);

  const fontSize = getFontSize(keyType, label);

  return (
    <>
      {/* Secondary labels above the key */}
      {(secondLabel || alphaLabel) && (
        <div
          style={{
            position: 'absolute',
            left: x - 1,
            top: y - 7,
            width: w + 2,
            height: 6,
            display: 'flex',
            alignItems: 'flex-end',
            justifyContent: secondLabel && alphaLabel ? 'space-between' : 'center',
            pointerEvents: 'none',
            overflow: 'hidden',
          }}
        >
          {secondLabel && (
            <span
              style={{
                fontSize: 3,
                lineHeight: 1,
                color: secondColor,
                whiteSpace: 'nowrap',
                letterSpacing: '-0.02em',
              }}
            >
              {secondLabel}
            </span>
          )}
          {alphaLabel && (
            <span
              style={{
                fontSize: 3,
                lineHeight: 1,
                color: alphaColor,
                whiteSpace: 'nowrap',
              }}
            >
              {alphaLabel}
            </span>
          )}
        </div>
      )}

      {/* Key button */}
      <button
        style={{
          position: 'absolute',
          left: x,
          top: y,
          width: w,
          height: h,
          borderRadius: 4,
          border: `${KEY_BORDER_WIDTH}px solid ${KEY_BORDER_COLOR}`,
          background: fillColor,
          color: textColor,
          fontSize,
          fontFamily: 'inherit',
          fontWeight: 700,
          lineHeight: 1,
          padding: 0,
          margin: 0,
          cursor: 'pointer',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          touchAction: 'manipulation',
          userSelect: 'none',
          WebkitUserSelect: 'none',
          outline: 'none',
          overflow: 'hidden',
          whiteSpace: 'nowrap',
          textTransform: 'lowercase',
          letterSpacing: '-0.02em',
        }}
        onMouseDown={handleDown}
        onMouseUp={handleUp}
        onMouseLeave={handleUp}
        onTouchStart={handleDown}
        onTouchEnd={handleUp}
        onTouchCancel={handleUp}
      >
        {label}
        {pressed && (
          <div
            style={{
              position: 'absolute',
              inset: 0,
              background: PRESSED_OVERLAY,
              borderRadius: 4,
              pointerEvents: 'none',
            }}
          />
        )}
      </button>
    </>
  );
}
