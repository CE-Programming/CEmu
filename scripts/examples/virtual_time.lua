-- Schedule work against emulated time rather than wall-clock time.
-- Timers advance only while the calculator is running.
if stopVirtualTimeExample then
    stopVirtualTimeExample()
    return
end

local timerIds = {}
local ticks = 0

timerIds[#timerIds + 1] = emu.after(500, function(event)
    print(string.format("one-shot at %.2f ms (%d cycles, %.2f ms late)",
                        event.time, event.cycles, event.late))
end)

timerIds[#timerIds + 1] = emu.every(250, function(event)
    ticks = ticks + 1
    print(string.format("virtual tick %d at %.2f ms", ticks, event.time))
    return ticks < 4 -- Returning false cancels this repeating timer.
end)

function stopVirtualTimeExample()
    for _, id in ipairs(timerIds) do emu.cancel(id) end
    stopVirtualTimeExample = nil
    print("Virtual-time timers cancelled")
end

print("Virtual-time timers installed; run this script again to cancel them")
