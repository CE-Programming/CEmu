export interface WebCEmuModule {
  FS: {
    writeFile(path: string, data: Uint8Array): void;
    readFile(path: string): Uint8Array;
    unlink(path: string): void;
    mkdir(path: string): void;
  };
  callMain(args?: string[]): void;
  ccall(ident: string, returnType: string | null, argTypes: string[], args: unknown[]): unknown;
  cwrap(ident: string, returnType: string | null, argTypes: string[]): (...args: unknown[]) => unknown;
  HEAPU8: Uint8Array;
  HEAPU32: Uint32Array;
  _malloc(size: number): number;
  _free(ptr: number): void;
}

export type WebCEmuFactory = (config?: Partial<WebCEmuModuleConfig>) => Promise<WebCEmuModule>;

export interface WebCEmuModuleConfig {
  locateFile(path: string, prefix: string): string;
  print(text: string): void;
  printErr(text: string): void;
}
