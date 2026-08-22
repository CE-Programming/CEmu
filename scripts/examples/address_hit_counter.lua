-- Count execution of a hot address without invoking Lua for every hit.
-- Replace this with the instruction or branch target you want to measure.
local address = cpu.registers.PC
local counter = dbg.hitCounter(address)
local totalHits = 0

print(("Counting executions of $%06X"):format(address))

local reportTimer = emu.every(1000, function()
    local hits = counter:reset()
    totalHits = totalHits + hits
    print(("$%06X: %d hits in the last emulated second"):format(address, hits))
end)

cemu.onUnload(function()
    emu.cancel(reportTimer)
    totalHits = totalHits + counter:reset()
    if counter.active then
        print(("$%06X: %d total hits"):format(address, totalHits))
    end
end)
