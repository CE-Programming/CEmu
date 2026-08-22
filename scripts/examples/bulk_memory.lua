-- Inspect a memory range with the bulk APIs without changing calculator state.
local startAddress = 0xD00000
local length = 256

local data = mem.read(startAddress, length)
local bytes = mem.readTable(startAddress, 16)
local firstLine = {}
for index, value in ipairs(bytes) do
    firstLine[index] = string.format("%02X", value)
end

print(string.format("%06X: %s", startAddress, table.concat(firstLine, " ")))
print(string.format("%d bytes, CRC-32=%08X", #data, mem.crc32(startAddress, length)))

local pattern = data:sub(1, 4)
local matches = mem.search(startAddress, length, pattern, 8)
for index, address in ipairs(matches) do
    print(string.format("pattern match %d at %06X", index, address))
end
