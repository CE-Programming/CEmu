-- Inspect the rendered LCD and sample the next completed frame.
local info = lcd.frameInfo()
local center = lcd.pixel(math.floor(lcd.width / 2), math.floor(lcd.height / 2))
local sample = lcd.region(0, 0, 8, 8, "rgb")

print(string.format("frame %dx%d %s: %d bytes, CRC-32=%08X",
                    info.width, info.height, info.format, info.bytes, info.crc32))
print(string.format("display enabled=%s DMA=%s backlight=%.3f center=%06X",
                    tostring(info.enabled), tostring(info.dma), info.backlight, center))
print(string.format("top-left 8x8 RGB sample: %d bytes", #sample))

cemu.on("frame", function(event)
    print(string.format("next frame: %.1f FPS, CRC-32=%08X, sequence=%d",
                        event.fps, lcd.frameHash(), event.sequence))
end, { once = true })
