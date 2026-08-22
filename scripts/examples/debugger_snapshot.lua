-- Capture debugger configuration and CPU state without changing it.
local registers = dbg.registerSnapshot()
print(string.format("PC=%06X AF=%06X BC=%06X DE=%06X HL=%06X ADL=%s",
                    registers.pc, registers.af, registers.bc, registers.de,
                    registers.hl, tostring(registers.adl)))

local breakpoints = dbg.breakpoints()
local watchpoints = dbg.watchpoints()
local portMonitors = dbg.peripheralMonitors()
local registerWatches = dbg.registerWatches()
print(string.format("debugger: %d breakpoints, %d watchpoints, %d port monitors, %d register watches",
                    #breakpoints, #watchpoints, #portMonitors, #registerWatches))

local equates = dbg.equates()
print(string.format("loaded equates: %d", #equates))
for index = 1, math.min(#equates, 10) do
    print(string.format("  %-24s = %06X", equates[index].name, equates[index].address))
end

local instruction = dbg.disasmPC()
print(string.format("next instruction: %06X  %s", instruction.address, instruction.text))
