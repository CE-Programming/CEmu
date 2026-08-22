-- Mirror a Port Monitor row in Lua and log matching CPU-style accesses.
-- Change the offset for the specific register you want to observe.
if stopPeripheralMonitor then
    stopPeripheralMonitor()
    return
end

local address = peripherals.ranges.lcd.base
peripherals.monitor(address, true, true, false)

local readHandler = cemu.on("peripheral-read", function(event)
    if event.address ~= address then return true end
    print(string.format("port read  %04X -> %02X, PC=%06X", event.address, event.value, event.pc))
    return false
end)

local writeHandler = cemu.on("peripheral-write", function(event)
    if event.address ~= address then return true end
    print(string.format("port write %04X -> %02X, PC=%06X", event.address, event.value, event.pc))
    return false
end)

function stopPeripheralMonitor()
    peripherals.monitor(address, false, false, false)
    cemu.off("peripheral-read", readHandler)
    cemu.off("peripheral-write", writeHandler)
    stopPeripheralMonitor = nil
    print("Peripheral monitor removed")
end

print(string.format("Monitoring LCD peripheral register %04X; run this script again to remove it", address))
