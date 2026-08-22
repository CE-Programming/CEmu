-- Monitor calculator file transfers. This script does not start one itself.
if stopTransferMonitor then
    stopTransferMonitor()
    return
end

local handlers = {}
handlers["transfer-start"] = cemu.on("transfer-start", function(event)
    print(string.format("transfer started: %d file(s), location=%d", #event.files, event.location))
end)
handlers["transfer-progress"] = cemu.on("transfer-progress", function(event)
    print(string.format("transfer progress: %d/%d (%.1f%%)",
                        event.amount, event.total, event.fraction * 100))
end)
handlers["transfer-complete"] = cemu.on("transfer-complete", function(event)
    print(string.format("transfer complete: %d/%d", event.amount, event.total))
end)
handlers["transfer-error"] = cemu.on("transfer-error", function(event)
    print(string.format("transfer %s: %d/%d",
                        event.cancelled and "cancelled" or "failed", event.amount, event.total))
end)

local status = link.status()
print(string.format("transfer monitor installed; current status=%s, busy=%s",
                    status.status, tostring(status.busy)))

function stopTransferMonitor()
    for event, handler in pairs(handlers) do cemu.off(event, handler) end
    stopTransferMonitor = nil
    print("Transfer monitor removed")
end
