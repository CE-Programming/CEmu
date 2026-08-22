-- Build a tokenized TI-BASIC program on the host, then transfer it to RAM.
-- Change archived to true and omit the send location to make Archive the default.
local program = tivars.create("Program", "LUAHELLO", [[
ClrHome
Disp "HELLO FROM LUA"
]], {
    archived = false,
})

print(string.format("Created %s.%s for %s (%d-byte file)",
                    program.name, program.extension, program.model, #program:bytes()))

if program:send("ram") then
    print("Transfer started")
else
    print("Transfer is already busy")
end
