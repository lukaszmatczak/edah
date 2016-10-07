--[[
Edah
Copyright (C) 2015-2016  Lukasz Matczak

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
]]

utils = require "mp.utils"

function send_state()
    local data = {currPos = mp.get_property_number("time-pos", 0),
        pause = mp.get_property("pause", "yes"),
        peakLevel = 0.0}
        
    local astats = mp.get_property_native("af-metadata/level")

    if astats ~= nil then
        data.peakLevel = tonumber(astats["lavfi.astats.Overall.Max_level"])
    end

    print(tostring(utils.format_json(data)))
end

function audio_reconf()
    local data = { audioDev =  mp.get_property_native("audio-device-list"),
        empty = 0 }
    print(tostring(utils.format_json(data)))
end

function end_file(event)
    if event.reason == "eof" then
        local data = { event = "end_file",
            empty = 0 }
        print(tostring(utils.format_json(data)))
    end
    
end

mp.add_periodic_timer(0.1, send_state)
mp.register_event("end-file", end_file)
mp.register_event("audio-reconfig", audio_reconf)

mp.add_timeout(0.2, audio_reconf)
