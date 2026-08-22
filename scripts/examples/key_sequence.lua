-- Drive the keypad with individual actions and a deterministic sequence.
keys.press("clear")
keys.hold("2nd", 100)
keys.sequence("mode,delay:150,clear,delay:150,2nd,delay:100,mode")

print("Key sequence queued")
