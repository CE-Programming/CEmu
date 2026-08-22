-- A small deterministic-input/TAS starting point.
-- Key names and delay/hold steps use the same grammar as CEmu's --keys option.
local ok, message = pcall(function()
    emu.throttle(false)
    keys.sequence("clear,delay:100,prgm,delay:100,enter,delay:500")
    assert(gui.screenshot("tas-demo.png"), "could not save tas-demo.png")
end)

-- Always restore normal throttling, even when a TAS step fails.
emu.throttle(true)
if not ok then error(message) end
