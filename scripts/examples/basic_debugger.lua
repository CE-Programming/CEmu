-- Configure TI-BASIC source debugging and report source-level stops.
-- Start a TI-BASIC program after running this script.
if stopBasicExample then
    stopBasicExample()
    return
end

basic.enable(true)
basic.setHighlight(true)
basic.setLiveExecution(true)

local breakpointHandler = cemu.on("basic-breakpoint", function(event)
    print(string.format("BASIC breakpoint: %s line %d byte %d PC=%06X",
                        event.program, event.line, event.byteOffset, event.pc))
    return true -- Keep the emulator paused so the source debugger can be inspected.
end)

local variableHandler = cemu.on("basic-variable-change", function(event)
    print("BASIC variables changed in " .. event.program .. ": " .. table.concat(event.variables, ", "))
    return true
end)

function stopBasicExample()
    cemu.off("basic-breakpoint", breakpointHandler)
    cemu.off("basic-variable-change", variableHandler)
    basic.enable(false)
    stopBasicExample = nil
    print("TI-BASIC example disabled")
end

print("TI-BASIC debugging enabled; run this script again to disable it")
