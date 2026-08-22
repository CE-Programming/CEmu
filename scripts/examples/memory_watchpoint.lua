-- Log writes to a RAM range and immediately resume instead of opening the debugger.
if stopMemoryWatchpoint then
    stopMemoryWatchpoint()
    return
end

local firstAddress = 0xD00000
local lastAddress = firstAddress + 0x0F

dbg.addWatchpoint(firstAddress, lastAddress, false, true, "Lua RAM logger")
local handler = cemu.on("watchpoint", function(event)
    if event.address < firstAddress or event.address > lastAddress then return true end
    print(string.format("RAM write at %06X, PC=%06X", event.address, event.pc))
    return false
end)

function stopMemoryWatchpoint()
    cemu.off("watchpoint", handler)
    dbg.removeWatchpoint(firstAddress)
    stopMemoryWatchpoint = nil
    print("Memory watchpoint removed")
end

print(string.format("Watching writes to %06X-%06X; run this script again to remove it",
                    firstAddress, lastAddress))
