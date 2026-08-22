-- Produce a read-only snapshot equivalent to the useful LCD debug-pane fields.
local state = lcd.state()
local controller = state.controller
local panel = state.panel

print(string.format("PL111 DMA=%06X row=%d column=%d phase=%d",
                    controller.upperCurrent, controller.currentRow,
                    controller.currentColumn, controller.phase))
print(string.format("ST7789 command=%02X window=(%d,%d)-(%d,%d)",
                    panel.command, panel.columnStart, panel.rowStart,
                    panel.columnEnd, panel.rowEnd))
print(string.format("panel clock=%d/%d backlight=%d (%.3f)",
                    panel.clockRate, panel.clockDivider,
                    state.backlight.brightness, state.backlight.factor))

local gammaFields = {
    "v0", "v1", "v2", "v4", "v6", "v13", "v20", "v27", "v36",
    "v43", "v50", "v57", "v59", "v61", "v62", "v63", "j0", "j1"
}

local function gammaString(gamma)
    local values = {}
    for _, field in ipairs(gammaFields) do
        values[#values + 1] = field .. "=" .. gamma[field]
    end
    return table.concat(values, ", ")
end

print("positive gamma: " .. gammaString(panel.positiveGamma))
print("negative gamma: " .. gammaString(panel.negativeGamma))
