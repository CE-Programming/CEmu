// CEmu-compliant keypad layout
// All positions from gui/qt/keypad/ source — 162×238 base coordinate space
// Row/col values map to C emulator emu_keypad_event — DO NOT CHANGE

export type KeyType = 'graph' | 'second' | 'alpha' | 'num' | 'other' | 'oper';

export interface KeyDef {
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
}

export interface ArrowKeyDef {
  row: number;
  col: number;
  direction: 'up' | 'down' | 'left' | 'right';
}

export const BASE_W = 162;
export const BASE_H = 238;

// D-pad: outer 35×35, inner 16×16, centered at (121, 53)
export const DPAD = { cx: 121, cy: 53, outer: 35, inner: 16 };

export const ARROW_KEYS: ArrowKeyDef[] = [
  { row: 7, col: 0, direction: 'down' },
  { row: 7, col: 1, direction: 'left' },
  { row: 7, col: 2, direction: 'right' },
  { row: 7, col: 3, direction: 'up' },
];

// All rectangular keys — positions in 162×238 base space
// Organized by emulator row for clarity
export const KEYS: KeyDef[] = [
  // ── Row 1: graph keys (y=14, h=9) + 2nd/mode/del (y=37, h=12) ──
  { label: 'y=',     row: 1, col: 4, x: 18,  y: 14,  w: 18, h: 9,  keyType: 'graph',  secondLabel: 'stat plot', alphaLabel: 'f1' },
  { label: 'window', row: 1, col: 3, x: 45,  y: 14,  w: 18, h: 9,  keyType: 'graph',  secondLabel: 'tblset',    alphaLabel: 'f2' },
  { label: 'zoom',   row: 1, col: 2, x: 72,  y: 14,  w: 18, h: 9,  keyType: 'graph',  secondLabel: 'format',    alphaLabel: 'f3' },
  { label: 'trace',  row: 1, col: 1, x: 99,  y: 14,  w: 18, h: 9,  keyType: 'graph',  secondLabel: 'calc',      alphaLabel: 'f4' },
  { label: 'graph',  row: 1, col: 0, x: 126, y: 14,  w: 18, h: 9,  keyType: 'graph',  secondLabel: 'table',     alphaLabel: 'f5' },
  { label: '2nd',    row: 1, col: 5, x: 18,  y: 37,  w: 18, h: 12, keyType: 'second' },
  { label: 'mode',   row: 1, col: 6, x: 45,  y: 37,  w: 18, h: 12, keyType: 'other',  secondLabel: 'quit' },
  { label: 'del',    row: 1, col: 7, x: 72,  y: 37,  w: 18, h: 12, keyType: 'other',  secondLabel: 'ins' },

  // ── Row 2 (x=18 column): alpha→on (top to bottom) ──
  { label: 'alpha',  row: 2, col: 7, x: 18,  y: 58,  w: 18, h: 12, keyType: 'alpha',  secondLabel: 'A-lock' },
  { label: 'math',   row: 2, col: 6, x: 18,  y: 79,  w: 18, h: 12, keyType: 'other',  secondLabel: 'test',   alphaLabel: 'A' },
  { label: 'x⁻¹',   row: 2, col: 5, x: 18,  y: 100, w: 18, h: 12, keyType: 'other',  secondLabel: 'matrix', alphaLabel: 'D' },
  { label: 'x²',     row: 2, col: 4, x: 18,  y: 121, w: 18, h: 12, keyType: 'other',  secondLabel: '√‾‾',   alphaLabel: 'I' },
  { label: 'log',    row: 2, col: 3, x: 18,  y: 142, w: 18, h: 12, keyType: 'other',  secondLabel: '10ˣ',   alphaLabel: 'N' },
  { label: 'ln',     row: 2, col: 2, x: 18,  y: 163, w: 18, h: 12, keyType: 'other',  secondLabel: 'eˣ',    alphaLabel: 'S' },
  { label: 'sto→',   row: 2, col: 1, x: 18,  y: 184, w: 18, h: 12, keyType: 'other',  secondLabel: 'rcl',   alphaLabel: 'X' },
  { label: 'on',     row: 2, col: 0, x: 18,  y: 205, w: 18, h: 12, keyType: 'other',  secondLabel: 'off' },

  // ── Row 3 (x=45 column): X,T,θ,n→0 ──
  { label: 'X,T,θ,n', row: 3, col: 7, x: 45, y: 58,  w: 18, h: 12, keyType: 'other', secondLabel: 'link' },
  { label: 'apps',    row: 3, col: 6, x: 45, y: 79,  w: 18, h: 12, keyType: 'other', secondLabel: 'angle',  alphaLabel: 'B' },
  { label: 'sin',     row: 3, col: 5, x: 45, y: 100, w: 18, h: 12, keyType: 'other', secondLabel: 'sin⁻¹', alphaLabel: 'E' },
  { label: ',',       row: 3, col: 4, x: 45, y: 121, w: 18, h: 12, keyType: 'other', secondLabel: 'EE',    alphaLabel: 'J' },
  { label: '7',       row: 3, col: 3, x: 45, y: 142, w: 18, h: 14, keyType: 'num',   secondLabel: 'u',     alphaLabel: 'O' },
  { label: '4',       row: 3, col: 2, x: 45, y: 165, w: 18, h: 14, keyType: 'num',   secondLabel: 'L4',    alphaLabel: 'T' },
  { label: '1',       row: 3, col: 1, x: 45, y: 188, w: 18, h: 14, keyType: 'num',   secondLabel: 'L1',    alphaLabel: 'Y' },
  { label: '0',       row: 3, col: 0, x: 45, y: 211, w: 18, h: 14, keyType: 'num',   secondLabel: 'catalog', alphaLabel: '_' },

  // ── Row 4 (x=72 column): stat→. ──
  { label: 'stat',   row: 4, col: 7, x: 72, y: 58,  w: 18, h: 12, keyType: 'other', secondLabel: 'list' },
  { label: 'prgm',   row: 4, col: 6, x: 72, y: 79,  w: 18, h: 12, keyType: 'other', secondLabel: 'draw',   alphaLabel: 'C' },
  { label: 'cos',    row: 4, col: 5, x: 72, y: 100, w: 18, h: 12, keyType: 'other', secondLabel: 'cos⁻¹', alphaLabel: 'F' },
  { label: '(',      row: 4, col: 4, x: 72, y: 121, w: 18, h: 12, keyType: 'other', secondLabel: '{',     alphaLabel: 'K' },
  { label: '8',      row: 4, col: 3, x: 72, y: 142, w: 18, h: 14, keyType: 'num',   secondLabel: 'v',     alphaLabel: 'P' },
  { label: '5',      row: 4, col: 2, x: 72, y: 165, w: 18, h: 14, keyType: 'num',   secondLabel: 'L5',    alphaLabel: 'U' },
  { label: '2',      row: 4, col: 1, x: 72, y: 188, w: 18, h: 14, keyType: 'num',   secondLabel: 'L2',    alphaLabel: 'Z' },
  { label: '.',      row: 4, col: 0, x: 72, y: 211, w: 18, h: 14, keyType: 'num',   secondLabel: 'i',     alphaLabel: ':' },

  // ── Row 5 (x=99 column): vars→(-) ──
  { label: 'vars',   row: 5, col: 6, x: 99, y: 79,  w: 18, h: 12, keyType: 'other', secondLabel: 'distr' },
  { label: 'tan',    row: 5, col: 5, x: 99, y: 100, w: 18, h: 12, keyType: 'other', secondLabel: 'tan⁻¹', alphaLabel: 'G' },
  { label: ')',      row: 5, col: 4, x: 99, y: 121, w: 18, h: 12, keyType: 'other', secondLabel: '}',     alphaLabel: 'L' },
  { label: '9',      row: 5, col: 3, x: 99, y: 142, w: 18, h: 14, keyType: 'num',   secondLabel: 'w',     alphaLabel: 'Q' },
  { label: '6',      row: 5, col: 2, x: 99, y: 165, w: 18, h: 14, keyType: 'num',   secondLabel: 'L6',    alphaLabel: 'V' },
  { label: '3',      row: 5, col: 1, x: 99, y: 188, w: 18, h: 14, keyType: 'num',   secondLabel: 'L3',    alphaLabel: 'θ' },
  { label: '(-)',    row: 5, col: 0, x: 99, y: 211, w: 18, h: 14, keyType: 'num',   secondLabel: 'ans',   alphaLabel: '?' },

  // ── Row 6 (x=126 column): clear→enter ──
  { label: 'clear',  row: 6, col: 6, x: 126, y: 79,  w: 18, h: 12, keyType: 'other' },
  { label: '^',      row: 6, col: 5, x: 126, y: 100, w: 18, h: 12, keyType: 'other', secondLabel: 'π',    alphaLabel: 'H' },
  { label: '÷',      row: 6, col: 4, x: 126, y: 121, w: 18, h: 12, keyType: 'oper',  secondLabel: 'e',    alphaLabel: 'M' },
  { label: '×',      row: 6, col: 3, x: 126, y: 142, w: 18, h: 12, keyType: 'oper',  secondLabel: '[',    alphaLabel: 'R' },
  { label: '—',      row: 6, col: 2, x: 126, y: 163, w: 18, h: 12, keyType: 'oper',  secondLabel: ']',    alphaLabel: 'W' },
  { label: '+',      row: 6, col: 1, x: 126, y: 184, w: 18, h: 12, keyType: 'oper',  secondLabel: 'mem',  alphaLabel: '"' },
  { label: 'enter',  row: 6, col: 0, x: 126, y: 205, w: 18, h: 12, keyType: 'oper',  secondLabel: 'entry', alphaLabel: 'solve' },
];
