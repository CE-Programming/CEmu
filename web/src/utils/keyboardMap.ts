export interface KeyMapping {
  row: number;
  col: number;
}

export const KEYBOARD_MAP: Record<string, KeyMapping> = {
  // Number keys
  'Digit0': { row: 3, col: 0 },
  'Digit1': { row: 3, col: 1 },
  'Digit2': { row: 4, col: 1 },
  'Digit3': { row: 5, col: 1 },
  'Digit4': { row: 3, col: 2 },
  'Digit5': { row: 4, col: 2 },
  'Digit6': { row: 5, col: 2 },
  'Digit7': { row: 3, col: 3 },
  'Digit8': { row: 4, col: 3 },
  'Digit9': { row: 5, col: 3 },

  // Arithmetic
  'NumpadAdd': { row: 6, col: 1 },
  'NumpadSubtract': { row: 6, col: 2 },
  'NumpadMultiply': { row: 6, col: 3 },
  'NumpadDivide': { row: 6, col: 4 },

  // Enter
  'Enter': { row: 6, col: 0 },
  'NumpadEnter': { row: 6, col: 0 },

  // Arrow keys
  'ArrowUp': { row: 7, col: 3 },
  'ArrowDown': { row: 7, col: 0 },
  'ArrowLeft': { row: 7, col: 1 },
  'ArrowRight': { row: 7, col: 2 },

  // Function keys / special
  'Backspace': { row: 1, col: 7 },  // del
  'Delete': { row: 6, col: 6 },     // clear
  'Escape': { row: 6, col: 6 },     // clear
  'Period': { row: 4, col: 0 },     // .
  'NumpadDecimal': { row: 4, col: 0 },
  'Comma': { row: 3, col: 4 },
  'Minus': { row: 5, col: 0 },      // negate
  'Equal': { row: 6, col: 1 },      // + (for convenience)
  'Slash': { row: 6, col: 4 },      // divide

  // Parentheses
  'BracketLeft': { row: 4, col: 4 },   // (
  'BracketRight': { row: 5, col: 4 },  // )

  // Calculator-specific
  'KeyA': { row: 2, col: 7 },  // alpha
  'KeyS': { row: 2, col: 1 },  // sto
  'KeyL': { row: 2, col: 3 },  // log
  'KeyN': { row: 2, col: 2 },  // ln
  'KeyM': { row: 2, col: 6 },  // math
  'KeyG': { row: 1, col: 0 },  // graph
  'KeyT': { row: 1, col: 1 },  // trace
  'KeyZ': { row: 1, col: 2 },  // zoom
  'KeyW': { row: 1, col: 3 },  // window
  'KeyY': { row: 1, col: 4 },  // y=
  'Tab': { row: 1, col: 5 },   // 2nd
  'CapsLock': { row: 1, col: 6 }, // mode
};
