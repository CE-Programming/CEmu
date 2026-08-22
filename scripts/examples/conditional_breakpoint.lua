-- Stop at the breakpoint only when the selected memory byte equals 1.
-- Returning false from a breakpoint callback resumes emulation immediately.
if stopConditionalBreakpoint then
    stopConditionalBreakpoint()
    return
end

local breakpointAddress = 0xD1A881
local conditionAddress = 0xD00000

dbg.addBreakpoint(breakpointAddress, "Lua conditional")
local handler = cemu.on("breakpoint", function(event)
    if event.address ~= breakpointAddress then return true end
    return mem.readByte(conditionAddress) == 1
end)

function stopConditionalBreakpoint()
    cemu.off("breakpoint", handler)
    dbg.removeBreakpoint(breakpointAddress)
    stopConditionalBreakpoint = nil
    print("Conditional breakpoint removed")
end

print("Conditional breakpoint installed; run this script again to remove it")
