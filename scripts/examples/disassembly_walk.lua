-- Decode instructions starting at the current PC without changing execution state.
local address = cpu.registers.PC
local instructionCount = 12

for _ = 1, instructionCount do
    local instruction = dbg.disasm(address, true)
    print(string.format("%06X  %-12s %s",
                        instruction.address, instruction.bytes, instruction.text))
    if instruction.next == address then
        cErr("Decoder did not advance at", string.format("%06X", address))
        break
    end
    address = instruction.next
end
