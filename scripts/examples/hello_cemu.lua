-- A minimal CEmu script: read CPU state and write to the Lua console/status bar.
local registers = cpu.registers

print("Hello from CEmu Lua!")
print(string.format("device=%d PC=%06X ADL=%s cycles=%d",
                    emu.deviceType(), registers.PC, tostring(cpu.ADL), cpu.cycles))
gui.status("Hello from Lua at PC=" .. string.format("%06X", registers.PC))
