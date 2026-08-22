-- Register state-scoped cleanup for resources owned by a long-running script.
if stopLifecycleExample then
    stopLifecycleExample()
    return
end

local keyHandler = cemu.on("key", function(event)
    if event.pressed and not event["repeat"] then
        print(string.format("key activity at %.2f ms", event.time))
    end
end)

local heartbeat = emu.every(1000, function(event)
    print(string.format("lifecycle heartbeat at %.2f ms", event.time))
end)

local unloadHandler = cemu.onUnload(function(event)
    print("Lifecycle example unloaded: " .. event.reason)
end)

function stopLifecycleExample()
    cemu.off("key", keyHandler)
    emu.cancel(heartbeat)
    cemu.offUnload(unloadHandler)
    stopLifecycleExample = nil
    print("Lifecycle example resources released")
end

print("Lifecycle example installed; run it again for manual cleanup")
