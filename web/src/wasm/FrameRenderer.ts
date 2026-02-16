import type { WebCEmuModule } from '../types/wasm';
import type { EmulatorBridge } from './EmulatorBridge';

const LCD_WIDTH = 320;
const LCD_HEIGHT = 240;

export class FrameRenderer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private imageData: ImageData;
  private animFrameId: number | null = null;
  private running = false;

  constructor(
    canvas: HTMLCanvasElement,
    private module: WebCEmuModule,
    private bridge: EmulatorBridge
  ) {
    this.canvas = canvas;
    this.canvas.width = LCD_WIDTH;
    this.canvas.height = LCD_HEIGHT;
    this.ctx = canvas.getContext('2d', { alpha: false })!;
    this.imageData = this.ctx.createImageData(LCD_WIDTH, LCD_HEIGHT);
  }

  start(): void {
    if (this.running) return;
    this.running = true;
    this.tick();
  }

  stop(): void {
    this.running = false;
    if (this.animFrameId !== null) {
      cancelAnimationFrame(this.animFrameId);
      this.animFrameId = null;
    }
  }

  private tick = (): void => {
    if (!this.running) return;

    this.renderFrame();
    this.animFrameId = requestAnimationFrame(this.tick);
  };

  private renderFrame(): void {
    const ptr = this.bridge.getFramePointer();
    if (ptr === 0) return;

    // display is uint32[240][320] — 240 rows, 320 cols
    // Each uint32 pixel in WASM memory is 0xFFRRGGBB (stored LE as BB GG RR FF)
    // ImageData needs [RR, GG, BB, AA] per pixel
    const pixelCount = LCD_WIDTH * LCD_HEIGHT;
    const wasmU32 = new Uint32Array(this.module.HEAPU8.buffer, ptr, pixelCount);
    const imgBuf = new Uint32Array(this.imageData.data.buffer);

    for (let i = 0; i < pixelCount; i++) {
      const val = wasmU32[i];
      // WASM: 0xFFRRGGBB (as stored uint32)
      // In LE memory: bytes are BB GG RR FF
      // We need ImageData LE uint32: 0xAABBGGRR
      const r = (val >> 16) & 0xff;
      const g = (val >> 8) & 0xff;
      const b = val & 0xff;
      imgBuf[i] = 0xff000000 | (b << 16) | (g << 8) | r;
    }

    this.ctx.putImageData(this.imageData, 0, 0);
  }
}
