import type { WebCEmuModule } from '../types/wasm';

const ROM_PATH = '/CE.rom';
const STATE_PATH = '/ce_state.img';
const TRANSFER_DIR = '/tmp';

export class EmulatorBridge {
  private keypadEvent: (row: number, col: number, press: number) => void;
  private getFramePtr: () => number;
  private resetFn: () => void;
  private pauseFn: () => void;
  private resumeFn: () => void;
  private saveFn: (type: number, path: string) => number;
  private loadFn: (type: number, path: string) => number;
  private setFileToSend: (path: string) => void;
  private _booted = false;

  constructor(private module: WebCEmuModule) {
    this.keypadEvent = module.cwrap('emu_keypad_event', null, ['number', 'number', 'number']) as (row: number, col: number, press: number) => void;
    this.getFramePtr = module.cwrap('lcd_get_frame', 'number', []) as () => number;
    this.resetFn = module.cwrap('emu_reset', null, []) as () => void;
    this.pauseFn = module.cwrap('emsc_pause_main_loop', null, []) as () => void;
    this.resumeFn = module.cwrap('emsc_resume_main_loop', null, []) as () => void;
    this.saveFn = module.cwrap('emu_save', 'number', ['number', 'string']) as (type: number, path: string) => number;
    this.loadFn = module.cwrap('emu_load', 'number', ['number', 'string']) as (type: number, path: string) => number;
    this.setFileToSend = module.cwrap('set_file_to_send', null, ['string']) as (path: string) => void;

    try {
      this.module.FS.mkdir(TRANSFER_DIR);
    } catch {
      // directory may already exist
    }

  }

  get booted(): boolean {
    return this._booted;
  }

  loadRom(data: Uint8Array): boolean {
    this.module.FS.writeFile(ROM_PATH, data);

    // Define legacy globals expected by EM_ASM blocks in os-emscripten.c main()
    const w = window as unknown as Record<string, unknown>;
    w.emul_is_inited = false;
    w.emul_is_paused = true;
    w.initFuncs = () => {};
    w.initLCD = () => {};
    w.enableGUI = () => {};
    w.disableGUI = () => {};

    this.module.callMain([]);
    this._booted = (w.emul_is_inited as boolean) === true;
    return this._booted;
  }

  pressKey(row: number, col: number): void {
    this.keypadEvent(row, col, 1);
  }

  releaseKey(row: number, col: number): void {
    this.keypadEvent(row, col, 0);
  }

  getFramePointer(): number {
    return this.getFramePtr();
  }

  reset(): void {
    this.resetFn();
  }

  pause(): void {
    this.pauseFn();
  }

  resume(): void {
    this.resumeFn();
  }

  saveState(): Uint8Array | null {
    // EMU_DATA_IMAGE = 0
    const result = this.saveFn(0, STATE_PATH);
    if (!result) return null;
    try {
      const data = this.module.FS.readFile(STATE_PATH);
      this.module.FS.unlink(STATE_PATH);
      return data;
    } catch {
      return null;
    }
  }

  loadState(data: Uint8Array): boolean {
    this.module.FS.writeFile(STATE_PATH, data);
    // EMU_DATA_IMAGE = 0
    const result = this.loadFn(0, STATE_PATH);
    try {
      this.module.FS.unlink(STATE_PATH);
    } catch {
      // ignore
    }
    // EMU_STATE_VALID = 0
    return result === 0;
  }

  sendFile(name: string, data: Uint8Array): void {
    const path = `${TRANSFER_DIR}/${name}`;
    this.module.FS.writeFile(path, data);
    this.setFileToSend(path);
  }
}
