-- Inspect all core peripherals without depending on Qt widget internals.
for name, range in pairs(peripherals.ranges) do
    local first = peripherals.snapshot(range.base, 16)
    local bytes = {}
    for index, value in ipairs(first) do
        bytes[index] = string.format("%02X", value)
    end
    print(string.format("%-10s %04X: %s", name, range.base, table.concat(bytes, " ")))
end

local display = lcd.state()
print("PL111 DMA", string.format("%06X", display.controller.upperCurrent))
print("ST7789 window", display.panel.columnStart, display.panel.columnEnd,
      display.panel.rowStart, display.panel.rowEnd)
