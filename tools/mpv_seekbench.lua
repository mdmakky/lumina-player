-- mpv counterpart of Lumina's LUMINA_SEEK_BENCH: N exact + N keyframe seeks,
-- latency = seek command -> playback-restart. Usage:
--   mpv --no-config --script=tools/mpv_seekbench.lua \
--       --script-opts=seekbench-csv=/tmp/mpv.csv FILE
-- Add --hwdec=vaapi (etc.) as for any mpv run. Prints a SEEKBENCH line like Lumina.
local mp = require 'mp'
local N = tonumber(mp.get_opt('seekbench-n')) or 40
local csv_path = mp.get_opt('seekbench-csv')
local lat, rows, mode, i, t0, target = {exact = {}, keyframe = {}}, {}, 'exact', 0, 0, 0

local function pct(v, p)
    if #v == 0 then return 0 end
    table.sort(v)
    return v[math.max(1, math.ceil(p * #v))]
end

local function finish()
    local out = {}
    for _, m in ipairs({'exact', 'keyframe'}) do
        local v = lat[m]
        out[#out + 1] = string.format('%s[p50=%.1f p95=%.1f max=%.1f]', m, pct(v, 0.5), pct(v, 0.95), pct(v, 1.0))
    end
    print(string.format('SEEKBENCH n=%d hwdec=%s %s (ms)', N,
        tostring(mp.get_property('hwdec-current')), table.concat(out, ' ')))
    if csv_path then
        local f = io.open(csv_path, 'w')
        f:write('mode,index,target_s,latency_ms\n')
        for _, r in ipairs(rows) do f:write(r, '\n') end
        f:close()
    end
    mp.command('quit')
end

local function step()
    if i >= N then
        if mode == 'exact' then mode, i = 'keyframe', 0 else return finish() end
    end
    i = i + 1
    target = 1 + math.random() * (mp.get_property_number('duration') - 6)
    t0 = mp.get_time()
    mp.commandv('seek', string.format('%.3f', target),
        mode == 'exact' and 'absolute+exact' or 'absolute+keyframes')
end

mp.register_event('playback-restart', function()
    if t0 == 0 then return end
    local ms = (mp.get_time() - t0) * 1000
    t0 = 0
    table.insert(lat[mode], ms)
    rows[#rows + 1] = string.format('%s,%d,%.3f,%.1f', mode, i - 1, target, ms)
    mp.add_timeout(0.15, step)
end)

mp.register_event('file-loaded', function()
    math.randomseed(os.time())
    mp.add_timeout(1.0, step)
end)
