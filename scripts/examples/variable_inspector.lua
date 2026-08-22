-- List calculator variables and read one raw VAT entry without launching it.
local variables = vars.list()
print(string.format("VAT contains %d variables", #variables))

for index, variable in ipairs(variables) do
    print(string.format("%-8s %-12s %6d bytes at %06X%s",
                        variable.name, variable.type, variable.size, variable.address,
                        variable.archived and " (archived)" or ""))
    if index == 20 then
        print(string.format("... and %d more", #variables - index))
        break
    end
end

if variables[1] then
    local first = vars.find(variables[1].name, variables[1].typeId)
    local raw = vars.read(first.name, first.typeId)
    print(string.format("Read %s as %s: %d raw bytes",
                        first.name, first.normalizedType, #raw))
end

local types = vars.types()
print(string.format("CEmu recognizes %d calculator variable type IDs", #types))
