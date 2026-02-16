// CEmu-compliant skin engine
// Matches gui/qt/keypad/keypadwidget.cpp color schemes exactly

// ── Qt HSV color math (matching QColor::lighter / QColor::darker) ──

function hexToRgb(hex: string): [number, number, number] {
  const n = parseInt(hex.replace('#', ''), 16);
  return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
}

function rgbToHex(r: number, g: number, b: number): string {
  return '#' + [r, g, b].map(c => Math.round(c).toString(16).padStart(2, '0')).join('');
}

function rgbToHsv(r: number, g: number, b: number): [number, number, number] {
  const max = Math.max(r, g, b), min = Math.min(r, g, b);
  const d = max - min;
  const s = max === 0 ? 0 : (d / max) * 255;
  const v = max;
  let h = 0;
  if (d !== 0) {
    switch (max) {
      case r: h = ((g - b) / d + (g < b ? 6 : 0)) / 6; break;
      case g: h = ((b - r) / d + 2) / 6; break;
      case b: h = ((r - g) / d + 4) / 6; break;
    }
  }
  return [h * 360, s, v]; // Qt uses H=0-360, S=0-255, V=0-255
}

function hsvToRgb(h: number, s: number, v: number): [number, number, number] {
  h /= 360; s /= 255; v /= 255;
  let r = 0, g = 0, b = 0;
  if (s === 0) { r = g = b = v; }
  else {
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    switch (i % 6) {
      case 0: r = v; g = t; b = p; break;
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      case 5: r = v; g = p; b = q; break;
    }
  }
  return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

export function lighter(hex: string, factor: number): string {
  if (factor <= 0) return '#000000';
  if (factor < 100) return darker(hex, Math.round(10000 / factor));
  const [r, g, b] = hexToRgb(hex);
  let [h, s, v] = rgbToHsv(r, g, b);
  v = (v * factor) / 100;
  if (v > 255) {
    s -= v - 255;
    if (s < 0) s = 0;
    v = 255;
  }
  const [nr, ng, nb] = hsvToRgb(h, s, v);
  return rgbToHex(nr, ng, nb);
}

export function darker(hex: string, factor: number): string {
  if (factor <= 0) return '#000000';
  if (factor < 100) return lighter(hex, Math.round(10000 / factor));
  const [r, g, b] = hexToRgb(hex);
  let [h, s, v] = rgbToHsv(r, g, b);
  v = (v * 100) / factor;
  const [nr, ng, nb] = hsvToRgb(h, s, v);
  return rgbToHex(nr, ng, nb);
}

// ── Color scheme definitions ──

export interface ColorScheme {
  id: string;
  name: string;
  center: string;
  sides: string;
  num: string;
  text: string;
  other: string;
  graph: string;
  transparent?: boolean;
  centerAlpha?: number;
  otherAlpha?: number;
  graphAlpha?: number;
}

// Shared defaults (TI-84 PCE English)
const D = {
  num: '#eeeeee',
  text: '#eeeeee',
  graph: '#eeeeee',
};

export const SCHEMES: ColorScheme[] = [
  { id: 'black', name: 'Black',
    center: '#191919', sides: '#3b3b3b',
    ...D, other: '#1d1d1d' },
  { id: 'white', name: 'White',
    center: '#e8e8e8', sides: '#dddddd',
    num: '#707880', text: '#222222', other: '#c0c0c0', graph: '#eeeeee' },
  { id: 'true-blue', name: 'True Blue',
    center: '#385E9D', sides: lighter('#385E9D', 130),
    num: '#dedede', text: '#eeeeee', other: '#274F91', graph: '#eeeeee' },
  { id: 'denim', name: 'Denim',
    center: '#003C71', sides: lighter('#003C71', 130),
    ...D, other: '#013766' },
  { id: 'silver', name: 'Silver',
    center: '#7C878E', sides: lighter('#7C878E', 130),
    ...D, other: '#191919', graph: '#D0D3D4' },
  { id: 'pink', name: 'Pink',
    center: '#DF1995', sides: lighter('#DF1995', 130),
    ...D, other: '#AA0061' },
  { id: 'plum', name: 'Plum',
    center: '#830065', sides: lighter('#830065', 130),
    ...D, other: '#5E2751' },
  { id: 'red', name: 'Red',
    center: '#AB2328', sides: lighter('#AB2328', 130),
    ...D, other: '#8A2A2B' },
  { id: 'lightning', name: 'Lightning',
    center: '#0077C8', sides: lighter('#0077C8', 130),
    ...D, other: '#0077C8' },
  { id: 'golden', name: 'Golden',
    center: '#D8D3B6', sides: lighter('#D8D3B6', 130),
    ...D, other: '#D8D3B6' },
  { id: 'spacegrey', name: 'Space Grey',
    center: '#DBDBDB', sides: darker('#DBDBDB', 130),
    ...D, other: '#353535', graph: '#D0D3D4' },
  { id: 'coral', name: 'Coral',
    center: '#FD6D99', sides: lighter('#FD6D99', 120),
    ...D, other: '#353535', graph: '#D0D3D4' },
  { id: 'mint', name: 'Mint',
    center: '#D2EBE8', sides: darker('#D2EBE8', 115),
    ...D, other: '#353535', graph: '#D0D3D4' },
  { id: 'rosegold', name: 'Rose Gold',
    center: '#AF867C', sides: darker('#AF867C', 105),
    num: '#eeeeee', text: '#222222', other: '#D8D3B6', graph: '#D0D3D4' },
  { id: 'crystalclear', name: 'Crystal Clear',
    center: '#ACA7AE', sides: lighter('#ACA7AE', 130),
    ...D, other: '#191919', graph: '#D0D3D4',
    transparent: true, centerAlpha: 220, otherAlpha: 120, graphAlpha: 120 },
  { id: 'matteblack', name: 'Matte Black',
    center: '#0F0F0F', sides: '#0F0F0F',
    ...D, other: '#1d1d1d' },
  { id: 'tangentteal', name: 'Tangent Teal',
    center: '#005062', sides: lighter('#005062', 150),
    ...D, other: '#00272C', graph: '#6C7F90' },
  { id: 'totallyteal', name: 'Totally Teal',
    center: '#108798', sides: darker('#108798', 200),
    ...D, other: '#125E68', graph: '#2E4854' },
];

// ── Device type → skin image mapping ──

export type DeviceType = 'TI84PCE' | 'TI83PCE' | 'TI82AEP';

export interface DeviceConfig {
  type: DeviceType;
  python: boolean;
  secondColor: string;
  alphaColor: string;
  alphaKeyTextColor: string;
}

export function getSkinImage(device: DeviceType, python: boolean): string {
  switch (device) {
    case 'TI82AEP': return '/skin/ti82aep.png';
    case 'TI83PCE': return python ? '/skin/ti83pce_ep.png' : '/skin/ti83pce.png';
    case 'TI84PCE':
    default: return python ? '/skin/ti84pce_py.png' : '/skin/ti84pce.png';
  }
}

export function getDeviceConfig(device: DeviceType = 'TI84PCE', python = false): DeviceConfig {
  const is82 = device === 'TI82AEP';
  return {
    type: device,
    python,
    secondColor: is82 ? '#b02036' : '#93c3f3',
    alphaColor: is82 ? '#8cd0f6' : '#a0ca1e',
    alphaKeyTextColor: is82 ? '#222222' : '#eeeeee',
  };
}

// ── Shared constants ──

export const KEY_BORDER_COLOR = '#808080';
export const KEY_BORDER_WIDTH = 0.375;
export const BLACK_COLOR = '#222222';
export const WHITE_COLOR = '#eeeeee';
export const PRESSED_OVERLAY = 'rgba(128, 0, 0, 0.5)';

// ── Key color resolution (matches Qt key class constructors) ──

export function getKeyColors(
  keyType: 'graph' | 'second' | 'alpha' | 'num' | 'other' | 'oper',
  scheme: ColorScheme,
  device: DeviceConfig,
): { fill: string; text: string } {
  switch (keyType) {
    case 'num':    return { fill: scheme.num,          text: BLACK_COLOR };
    case 'other':  return { fill: scheme.other,        text: scheme.text };
    case 'oper':   return { fill: scheme.graph,        text: BLACK_COLOR };
    case 'graph':  return { fill: scheme.graph,        text: BLACK_COLOR };
    case 'second': return { fill: device.secondColor,  text: WHITE_COLOR };
    case 'alpha':  return { fill: device.alphaColor,   text: device.alphaKeyTextColor };
  }
}

// ── Skin image dimensions ──

export const SKIN_W = 440;
export const SKIN_H = 351;
export const LCD_X = 60;
export const LCD_Y = 78;
export const LCD_W = 320;
export const LCD_H = 240;
