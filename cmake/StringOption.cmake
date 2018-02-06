macro(string_option name help_message default)
    set(${name} "${default}" CACHE STRING ${help_message})
endmacro()