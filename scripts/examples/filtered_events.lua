-- Handle only the next non-repeating press in the top two keypad rows.
local handler = cemu.on("key", function(event)
    print(string.format("matched key row=%d column=%d at %.2f ms, PC=%06X, sequence=%d",
                        event.row, event.column, event.time, event.pc, event.sequence))
end, {
    once = true,
    pressed = true,
    predicate = function(event)
        return not event["repeat"] and event.row <= 1
    end
})

print(string.format("Filtered one-shot key handler %d installed", handler))
