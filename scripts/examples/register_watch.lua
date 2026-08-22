-- Mirror a CPU Register Watch row and log writes without leaving emulation paused.
if stopRegisterWatch then
    stopRegisterWatch()
    return
end

local registerName = "a"
dbg.watchRegister(registerName, false, true)

local handler = cemu.on("register-write", function(event)
    if event.name:lower() ~= registerName then return true end
    print(string.format("register %s written at PC=%06X", event.name, event.pc))
    return false
end)

function stopRegisterWatch()
    cemu.off("register-write", handler)
    dbg.watchRegister(registerName, false, false)
    stopRegisterWatch = nil
    print("Register watch removed")
end

print("Watching writes to register " .. registerName .. "; run this script again to remove it")
