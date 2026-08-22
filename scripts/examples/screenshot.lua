-- Save a timestamped PNG to the desktop using CEmu's default filename.
if gui.screenshot() then
    print("Screenshot saved")
else
    cErr("Screenshot could not be saved")
end
