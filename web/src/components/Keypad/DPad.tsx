import { useCallback, useState } from 'react';
import { DPAD, ARROW_KEYS } from './keypadLayout';
import { PRESSED_OVERLAY, KEY_BORDER_COLOR, KEY_BORDER_WIDTH } from '../../config/skinEngine';

interface Props {
  onPress: (row: number, col: number) => void;
  onRelease: (row: number, col: number) => void;
  keyColor: string;
  textColor: string;
}

const CX = DPAD.outer / 2;
const CY = DPAD.outer / 2;
const R_OUTER = DPAD.outer / 2;
const R_INNER = DPAD.inner / 2;
const MID_R = (R_INNER + R_OUTER) / 2;

const GAP = 6;
const SEGMENTS: { dir: 'up' | 'down' | 'left' | 'right'; s: number; e: number }[] = [
  { dir: 'up',    s: 225 + GAP / 2, e: 315 - GAP / 2 },
  { dir: 'right', s: 315 + GAP / 2, e: 405 - GAP / 2 },
  { dir: 'down',  s: 45  + GAP / 2, e: 135 - GAP / 2 },
  { dir: 'left',  s: 135 + GAP / 2, e: 225 - GAP / 2 },
];

function arcPath(startDeg: number, endDeg: number): string {
  const toRad = (d: number) => d * Math.PI / 180;
  const ox1 = CX + R_OUTER * Math.cos(toRad(startDeg));
  const oy1 = CY + R_OUTER * Math.sin(toRad(startDeg));
  const ox2 = CX + R_OUTER * Math.cos(toRad(endDeg));
  const oy2 = CY + R_OUTER * Math.sin(toRad(endDeg));
  const ix2 = CX + R_INNER * Math.cos(toRad(endDeg));
  const iy2 = CY + R_INNER * Math.sin(toRad(endDeg));
  const ix1 = CX + R_INNER * Math.cos(toRad(startDeg));
  const iy1 = CY + R_INNER * Math.sin(toRad(startDeg));
  return `M ${ox1} ${oy1} A ${R_OUTER} ${R_OUTER} 0 0 1 ${ox2} ${oy2} L ${ix2} ${iy2} A ${R_INNER} ${R_INNER} 0 0 0 ${ix1} ${iy1} Z`;
}

function arrowPath(dir: string): string {
  const sz = 2.5;
  switch (dir) {
    case 'up':    return `M ${CX} ${CY - MID_R - sz} L ${CX + sz} ${CY - MID_R + sz} L ${CX - sz} ${CY - MID_R + sz} Z`;
    case 'down':  return `M ${CX} ${CY + MID_R + sz} L ${CX + sz} ${CY + MID_R - sz} L ${CX - sz} ${CY + MID_R - sz} Z`;
    case 'left':  return `M ${CX - MID_R - sz} ${CY} L ${CX - MID_R + sz} ${CY - sz} L ${CX - MID_R + sz} ${CY + sz} Z`;
    case 'right': return `M ${CX + MID_R + sz} ${CY} L ${CX + MID_R - sz} ${CY - sz} L ${CX + MID_R - sz} ${CY + sz} Z`;
    default: return '';
  }
}

export default function DPad({ onPress, onRelease, keyColor, textColor }: Props) {
  const [pressed, setPressed] = useState<string | null>(null);

  const handleDown = useCallback((dir: string) => (e: React.MouseEvent | React.TouchEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setPressed(dir);
    const key = ARROW_KEYS.find(k => k.direction === dir);
    if (key) onPress(key.row, key.col);
  }, [onPress]);

  const handleUp = useCallback((dir: string) => (e: React.MouseEvent | React.TouchEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setPressed(null);
    const key = ARROW_KEYS.find(k => k.direction === dir);
    if (key) onRelease(key.row, key.col);
  }, [onRelease]);

  return (
    <svg
      viewBox={`0 0 ${DPAD.outer} ${DPAD.outer}`}
      style={{
        position: 'absolute',
        left: DPAD.cx - DPAD.outer / 2,
        top: DPAD.cy - DPAD.outer / 2,
        width: DPAD.outer,
        height: DPAD.outer,
        overflow: 'visible',
      }}
    >
      {/* Outer circle */}
      <circle
        cx={CX} cy={CY} r={R_OUTER}
        fill={keyColor}
        stroke={KEY_BORDER_COLOR}
        strokeWidth={KEY_BORDER_WIDTH}
      />

      {/* Arc segments — interactive */}
      {SEGMENTS.map(seg => (
        <g key={seg.dir}>
          <path
            d={arcPath(seg.s, seg.e)}
            fill={pressed === seg.dir ? PRESSED_OVERLAY : 'transparent'}
            cursor="pointer"
            style={{ touchAction: 'manipulation' }}
            onMouseDown={handleDown(seg.dir)}
            onMouseUp={handleUp(seg.dir)}
            onMouseLeave={handleUp(seg.dir)}
            onTouchStart={handleDown(seg.dir)}
            onTouchEnd={handleUp(seg.dir)}
            onTouchCancel={handleUp(seg.dir)}
          />
          <path
            d={arrowPath(seg.dir)}
            fill={textColor}
            pointerEvents="none"
          />
        </g>
      ))}

      {/* Inner circle */}
      <circle
        cx={CX} cy={CY} r={R_INNER}
        fill={keyColor}
        stroke={KEY_BORDER_COLOR}
        strokeWidth={KEY_BORDER_WIDTH}
      />
    </svg>
  );
}
