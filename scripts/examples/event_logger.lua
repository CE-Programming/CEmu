-- Demonstrates CEmu's generic Lua event hooks.
if stopEventLogger then
    stopEventLogger()
    return
end

local events = {
    "startup", "loaded", "reset", "key", "breakpoint", "watchpoint",
    "register-read", "register-write", "peripheral-read", "peripheral-write",
    "basic-breakpoint", "basic-step", "basic-variable-change", "script-loaded"
}

local handlers = {}
for _, event in ipairs(events) do
    local eventName = event
    handlers[eventName] = cemu.on(eventName, function(payload)
        print("event", eventName, payload.address or payload.path or "")
    end)
end

function stopEventLogger()
    for event, handler in pairs(handlers) do cemu.off(event, handler) end
    stopEventLogger = nil
    print("Event logger removed")
end

print("Event logger installed; run this script again to remove it")
