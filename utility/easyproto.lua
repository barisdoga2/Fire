-- EasyPacket Wireshark Dissector with multi-packet content decoding

local EASY = Proto("EasyPacket", "Easy UDP Game Packet")

-- Header fields
local f_session   = ProtoField.uint32("easypacket.session_id",  "Session ID",  base.DEC)
local f_sequence  = ProtoField.uint32("easypacket.sequence_id", "Sequence ID", base.DEC)
local f_iv        = ProtoField.bytes ("easypacket.iv",          "IV (Hex)")
local f_payload   = ProtoField.bytes ("easypacket.payload_raw", "Payload (Raw Hex)")
local f_tag       = ProtoField.bytes ("easypacket.tag",         "Tag (Hex)")

-- Payload generic
local f_pid       = ProtoField.uint16("easypacket.packet_id",   "Packet ID",   base.DEC)
local f_pid_name  = ProtoField.string("easypacket.packet_name", "Packet Name")

-- Common scalar fields
local f_bool      = ProtoField.bool  ("easypacket.bool",        "Bool")
local f_u32       = ProtoField.uint32("easypacket.u32",         "UInt32",      base.DEC)
local f_u64       = ProtoField.uint64("easypacket.u64",         "UInt64",      base.DEC)
local f_float     = ProtoField.float ("easypacket.float",       "Float")

-- Strings
local f_str       = ProtoField.string("easypacket.string",      "String")
local f_user      = ProtoField.string("easypacket.username",    "Username")
local f_msg       = ProtoField.string("easypacket.message",     "Message")

-- Specific packets
local f_login_resp   = ProtoField.bool  ("easypacket.login.response",  "Login Response")
local f_login_msg    = ProtoField.string("easypacket.login.message",   "Login Message")

local f_boot_userid  = ProtoField.uint32("easypacket.boot.userid",     "User ID",    base.DEC)
local f_boot_diam    = ProtoField.uint32("easypacket.boot.diamonds",   "Diamonds",   base.DEC)
local f_boot_gold    = ProtoField.uint32("easypacket.boot.golds",      "Golds",      base.DEC)
local f_boot_game    = ProtoField.uint32("easypacket.boot.gametime",   "Game Time",  base.DEC)
local f_boot_tut     = ProtoField.bool  ("easypacket.boot.tutorial",   "Tutorial Done")
local f_boot_champc  = ProtoField.uint64("easypacket.boot.champ_count","Champions Count")

local f_move_user    = ProtoField.uint32("easypacket.move.userid",     "User ID",    base.DEC)
local f_move_posx    = ProtoField.float ("easypacket.move.pos.x",      "Pos X")
local f_move_posy    = ProtoField.float ("easypacket.move.pos.y",      "Pos Y")
local f_move_posz    = ProtoField.float ("easypacket.move.pos.z",      "Pos Z")
local f_move_rotx    = ProtoField.float ("easypacket.move.rot.x",      "Rot X")
local f_move_roty    = ProtoField.float ("easypacket.move.rot.y",      "Rot Y")
local f_move_rotz    = ProtoField.float ("easypacket.move.rot.z",      "Rot Z")
local f_move_dirx    = ProtoField.float ("easypacket.move.dir.x",      "Dir X")
local f_move_diry    = ProtoField.float ("easypacket.move.dir.y",      "Dir Y")
local f_move_dirz    = ProtoField.float ("easypacket.move.dir.z",      "Dir Z")
local f_move_time    = ProtoField.uint64("easypacket.move.timestamp",  "Timestamp",  base.DEC)

local f_pack_count   = ProtoField.uint64("easypacket.movepack.count",  "Movement Count")

EASY.fields = {
    f_session, f_sequence, f_iv, f_payload, f_tag,
    f_pid, f_pid_name,
    f_bool, f_u32, f_u64, f_float,
    f_str, f_user, f_msg,
    f_login_resp, f_login_msg,
    f_boot_userid, f_boot_diam, f_boot_gold, f_boot_game, f_boot_tut, f_boot_champc,
    f_move_user, f_move_posx, f_move_posy, f_move_posz,
    f_move_rotx, f_move_roty, f_move_rotz,
    f_move_dirx, f_move_diry, f_move_dirz,
    f_move_time, f_pack_count
}

local IV_SIZE  = 8
local TAG_SIZE = 12

local PACKET_NAME = {
    [110] = "LOGIN_RESPONSE",
    [111] = "DISCONNECT_RESPONSE",
    [112] = "CHAMPION_SELECT_REQUEST",
    [113] = "CHAMPION_BUY_REQUEST",
    [114] = "CHAMPION_SELECT_RESPONSE",
    [115] = "CHAMPION_BUY_RESPONSE",
    [116] = "PLAYER_BOOT_INFO",
    [117] = "HEARTBEAT",
    [118] = "LOGIN_REQUEST",
    [119] = "LOGOUT_REQUEST",
    [120] = "BROADCAST_MESSAGE",
    [121] = "CHAT_MESSAGE",
    [122] = "PLAYER_MOVEMENT",
    [123] = "PLAYER_MOVEMENT_PACK"
}

local function safe_tvb(tvb, offset, len)
    local tvb_len = tvb:len()

    if offset < 0 then offset = 0 end
    if offset > tvb_len then offset = tvb_len end

    if len == nil or len < 0 then
        len = 0
    end

    if offset + len > tvb_len then
        len = tvb_len - offset
    end

    -- Wireshark için tvb(offset,0) GEÇERLİDİR, nil değildir.
    return tvb(offset, len)
end

local function find_next_pid(tvb, offset, limit)
    local pos = offset
    while pos + 2 <= limit do
        local maybe = tvb(pos,2):le_uint()
        if PACKET_NAME[maybe] ~= nil then
            return pos
        end
        pos = pos + 1
    end
    return limit
end

----------------------------------------------------------
-- helpers
----------------------------------------------------------
local function read_u16_le(tvb, offset)
    return safe_tvb(tvb, offset,2):le_uint(), offset+2
end

local function read_u32_le(tvb, offset)
    return safe_tvb(tvb, offset,4):le_uint(), offset+4
end

local function read_u64_le(tvb, offset)
    local u64 = safe_tvb(tvb, offset,8):le_uint64()
    return tonumber(u64), offset+8
end

local function read_bool(tvb, offset)
    local b = safe_tvb(tvb, offset,1):uint()
    return (b ~= 0), offset+1
end

local function read_float_le(tvb, offset)
    return safe_tvb(tvb, offset,4):le_float(), offset+4
end

local function read_size_t(tvb, offset)
    return read_u64_le(tvb, offset)
end

local function read_string(tvb, offset, tree, field)
    local len; len, offset = read_size_t(tvb, offset)
    if len < 0 then len = 0 end
    if offset + len > tvb:len() then len = math.max(0, tvb:len() - offset) end
    local str = safe_tvb(tvb, offset, len):string()
    tree:add(field or f_str, safe_tvb(tvb, offset, len), str)
    return str, offset + len
end

local function read_vec3(tvb, offset, subtree, prefix, fx, fy, fz)
    local x; x, offset = read_float_le(tvb, offset)
    local y; y, offset = read_float_le(tvb, offset)
    local z; z, offset = read_float_le(tvb, offset)

    if subtree then
        subtree:add(fx or f_float, safe_tvb(tvb, offset-12,4), x):set_text(prefix .. " X: " .. x)
        subtree:add(fy or f_float, safe_tvb(tvb, offset-8,4),  y):set_text(prefix .. " Y: " .. y)
        subtree:add(fz or f_float, safe_tvb(tvb, offset-4,4),  z):set_text(prefix .. " Z: " .. z)
    end

    return x, y, z, offset
end

----------------------------------------------------------
-- per-packet decoders
----------------------------------------------------------

local function decode_login_response(tvb, offset, tree, limit)
    if offset >= limit then return offset end
    local resp; resp, offset = read_bool(tvb, offset)
    tree:add(f_login_resp, safe_tvb(tvb, offset-1,1), resp)
    if offset >= limit then return offset end
    local msg;  msg,  offset = read_string(tvb, offset, tree, f_login_msg)
    return offset
end

local function decode_disconnect_response(tvb, offset, tree, limit)
    if offset >= limit then return offset end
    local _, new_off = read_string(tvb, offset, tree, f_msg)
    return new_off
end

local function decode_broadcast_message(tvb, offset, tree, limit)
    if offset >= limit then return offset end
    local _, new_off = read_string(tvb, offset, tree, f_msg)
    return new_off
end

local function decode_chat_message(tvb, offset, tree, limit)
    if offset >= limit then return offset end
    local msg; msg, offset = read_string(tvb, offset, tree, f_msg)
    if offset >= limit then return offset end
    local user; user, offset = read_string(tvb, offset, tree, f_user)
    if offset + 8 > limit then return offset end
    local ts; ts, offset = read_u64_le(tvb, offset)
    tree:add(f_move_time, safe_tvb(tvb, offset-8,8), ts)
    return offset
end

local function decode_player_boot_info(tvb, offset, tree, limit)
    if offset + 4 > limit then return offset end
    local uid; uid, offset = read_u32_le(tvb, offset)
    if offset + 12 > limit then return offset end
    local diam; diam, offset = read_u32_le(tvb, offset)
    local gold; gold, offset = read_u32_le(tvb, offset)
    local game; game, offset = read_u32_le(tvb, offset)
    if offset + 1 > limit then return offset end
    local tut; tut, offset = read_bool(tvb, offset)

    tree:add(f_boot_userid, safe_tvb(tvb, offset-17,4), uid)
    tree:add(f_boot_diam,   safe_tvb(tvb, offset-13,4), diam)
    tree:add(f_boot_gold,   safe_tvb(tvb, offset-9,4),  gold)
    tree:add(f_boot_game,   safe_tvb(tvb, offset-5,4),  game)
    tree:add(f_boot_tut,    safe_tvb(tvb, offset-1,1),  tut)

    if offset + 8 > limit then return offset end
    local count; count, offset = read_size_t(tvb, offset)
    tree:add(f_boot_champc, safe_tvb(tvb, offset-8,8), count)

    local max_bytes = math.min(limit-offset, count*4)
    local champs_node = tree:add(safe_tvb(tvb, offset, max_bytes), "Champions Owned")
    for i=0, count-1 do
        if offset + 4 > limit then break end
        local cid; cid, offset = read_u32_le(tvb, offset)
        champs_node:add(f_u32, safe_tvb(tvb, offset-4,4), cid):set_text("Champion["..i.."]: "..cid)
    end

    return offset
end

local function decode_player_movement(tvb, offset, tree, limit)
    if offset + 4 > limit then return offset end
    local uid; uid, offset = read_u32_le(tvb, offset)
    tree:add(f_move_user, safe_tvb(tvb, offset-4,4), uid)

    if offset + 12 > limit then return offset end
    local pos_node = tree:add(safe_tvb(tvb, offset,12), "Position")
    local _,_,_, off2 = read_vec3(tvb, offset, pos_node, "Pos", f_move_posx, f_move_posy, f_move_posz)
    offset = off2

    if offset + 12 > limit then return offset end
    local rot_node = tree:add(safe_tvb(tvb, offset,12), "Rotation")
    _,_,_, off2 = read_vec3(tvb, offset, rot_node, "Rot", f_move_rotx, f_move_roty, f_move_rotz)
    offset = off2

    if offset + 12 > limit then return offset end
    local dir_node = tree:add(safe_tvb(tvb, offset,12), "Direction")
    _,_,_, off2 = read_vec3(tvb, offset, dir_node, "Dir", f_move_dirx, f_move_diry, f_move_dirz)
    offset = off2

    if offset + 8 > limit then return offset end
    local ts; ts, offset = read_u64_le(tvb, offset)
    tree:add(f_move_time, safe_tvb(tvb, offset-8,8), ts)

    return offset
end

local function decode_player_movement_pack(tvb, offset, tree, limit)
    if offset + 8 > limit then return offset end
    local count; count, offset = read_size_t(tvb, offset)
    tree:add(f_pack_count, safe_tvb(tvb, offset-8,8), count)

    local node = tree:add(safe_tvb(tvb, offset, math.max(0,limit-offset)), "Movements")
    for i=0, count-1 do
        if offset >= limit then break end
        local mlen = math.min(48, limit-offset)
        local m_node = node:add(safe_tvb(tvb, offset, mlen), "Movement["..i.."]")
        offset = decode_player_movement(tvb, offset, m_node, limit)
    end

    return offset
end

----------------------------------------------------------
-- decode a single logical packet inside payload
----------------------------------------------------------
local function decode_one_packet(tvb, offset, tree, payload_len, index)
    if offset + 2 > payload_len then
        return offset
    end

    local start = offset
    local pid; pid, offset = read_u16_le(tvb, offset)
    local name = PACKET_NAME[pid] or "UNKNOWN"

    local next_start = find_next_pid(tvb, offset, payload_len)
	local pkt_body_len = next_start - start
	if pkt_body_len < 2 then pkt_body_len = 2 end  -- atleast header

	local pkt_tree = tree:add(safe_tvb(tvb, start, pkt_body_len),
							  string.format("Packet[%d]: %d (%s)", index, pid, name))

    pkt_tree:add(f_pid, safe_tvb(tvb, start,2), pid):append_text(" ("..name..")")
    pkt_tree:add(f_pid_name, safe_tvb(tvb, start,2), name)

    local limit = payload_len

    local function safe(fn)
        if offset >= limit then return offset end
        return fn()
    end

    if pid == 110 then
        offset = safe(function() return decode_login_response(tvb, offset, pkt_tree, limit) end)

    elseif pid == 111 then
        offset = safe(function() return decode_disconnect_response(tvb, offset, pkt_tree, limit) end)

    elseif pid == 112 or pid == 113 then
        offset = safe(function()
            if offset + 4 > limit then return offset end
            local cid; cid, offset2 = read_u32_le(tvb, offset)
            pkt_tree:add(f_u32, safe_tvb(tvb, offset2-4,4), cid):set_text("Champion ID: "..cid)
            return offset2
        end)

    elseif pid == 114 or pid == 115 then
        offset = safe(function() return decode_login_response(tvb, offset, pkt_tree, limit) end)

    elseif pid == 116 then
        offset = safe(function() return decode_player_boot_info(tvb, offset, pkt_tree, limit) end)

    elseif pid == 117 then
        -- HEARTBEAT: no body

    elseif pid == 118 or pid == 119 then
        -- LOGIN_REQUEST / LOGOUT_REQUEST: no body

    elseif pid == 120 then
        offset = safe(function() return decode_broadcast_message(tvb, offset, pkt_tree, limit) end)

    elseif pid == 121 then
        offset = safe(function() return decode_chat_message(tvb, offset, pkt_tree, limit) end)

    elseif pid == 122 then
        offset = safe(function() return decode_player_movement(tvb, offset, pkt_tree, limit) end)

    elseif pid == 123 then
        offset = safe(function() return decode_player_movement_pack(tvb, offset, pkt_tree, limit) end)

    else
        -- unknown: rest of payload from here
        pkt_tree:add(f_payload, safe_tvb(tvb, offset, limit-offset))
        offset = limit
    end

    return offset
end

----------------------------------------------------------
-- payload dispatcher: multiple logical packets per UDP
----------------------------------------------------------
local function dissect_payload(tvb, tree)
    local payload_len = tvb:len()
    if payload_len <= 0 then return end

    local offset = 0
    local idx = 0

    while offset + 2 <= payload_len do
        local prev = offset
        offset = decode_one_packet(tvb, offset, tree, payload_len, idx)
        idx = idx + 1
        if offset <= prev then
            -- safety to avoid infinite loop
            break
        end
    end
end

----------------------------------------------------------
-- main dissector
----------------------------------------------------------
local function dissect_easy(buffer, pinfo, tree)
    local total_len = buffer:len()
    if total_len < (4 + 4 + IV_SIZE) then
        return 0
    end

    pinfo.cols.protocol = "EASY"

    local subtree = tree:add(EASY, buffer(), "EasyPacket Protocol")

    local offset = 0

    subtree:add_le(f_session,  buffer(offset,4)); offset = offset + 4
    subtree:add_le(f_sequence, buffer(offset,4)); offset = offset + 4

    subtree:add(f_iv, buffer(offset, IV_SIZE))
    offset = offset + IV_SIZE

    local payload_len = total_len - offset - TAG_SIZE
    if payload_len < 0 then payload_len = 0 end

    local payload_tvb = buffer(offset, payload_len)
    subtree:add(f_payload, payload_tvb)
    offset = offset + payload_len

    local tag_tvb = buffer(offset, TAG_SIZE)
    subtree:add(f_tag, tag_tvb)

    local payload_tree = subtree:add(payload_tvb, "Decoded Payload")
    dissect_payload(payload_tvb, payload_tree)

    return total_len
end

function EASY.dissector(buffer, pinfo, tree)
    return dissect_easy(buffer, pinfo, tree)
end

local udp_port = DissectorTable.get("udp.port")
udp_port:add(54000, EASY)
