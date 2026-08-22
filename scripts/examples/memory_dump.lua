-- Read a small memory range without changing emulator state.
local startAddress = 0xD00000
local length = 64
local bytesPerLine = 16

for offset = 0, length - 1, bytesPerLine do
    local bytes = {}
    for column = 0, math.min(bytesPerLine - 1, length - offset - 1) do
        bytes[#bytes + 1] = string.format("%02X", mem.readByte(startAddress + offset + column))
    end
    print(string.format("%06X: %s", startAddress + offset, table.concat(bytes, " ")))
end
