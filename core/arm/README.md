[`CMSIS`](arm/CMSIS) and [`samd21a`](arm/samd21a) contain select files unzipped from [Atmel SAMD21 Series Device Support (1.3.395)](http://packs.download.atmel.com/#ATSAMD21E18A) and [CMSIS (Cortex Microcontroller Software Interface Standard) (5.4.0)](http://packs.download.atmel.com/#ARMCM0P) respectively and slightly modified.

The legacy SAMD21 header format is retained for its register bitfield types. USART `TXE`, `TXPO`, and `SAMPA` corrections from Microchip SAMD21_DFP 3.8.270 are backported locally.
