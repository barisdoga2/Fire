-- EasyPacket Wireshark Dissector

local EASY = Proto("EasyPacket", "Easy UDP Game Packet")

local f_session   = ProtoField.uint32("easypacket.session_id",  "Session ID",  base.DEC)
local f_sequence  = ProtoField.uint32("easypacket.sequence_id", "Sequence ID", base.DEC)
local f_iv        = ProtoField.bytes("easypacket.iv",           "IV (Hex)")
local f_payload   = ProtoField.bytes("easypacket.payload",      "Payload (Hex)")
local f_tag       = ProtoField.bytes("easypacket.tag",          "Tag (Hex)")

EASY.fields = { f_session, f_sequence, f_iv, f_payload, f_tag }

local IV_SIZE  = 12
local TAG_SIZE = 16

local function dissect_easy(buffer, pinfo, tree)
    local total_len = buffer:len()
    if total_len < (4 + 4 + IV_SIZE) then 
        return 0 
    end

    pinfo.cols.protocol = "EASY"

    local subtree = tree:add(EASY, buffer(), "EasyPacket Protocol")

    local offset = 0

    -- LITTLE-ENDIAN integers
    subtree:add_le(f_session, buffer(offset, 4))
    offset = offset + 4

    subtree:add_le(f_sequence, buffer(offset, 4))
    offset = offset + 4

    -- IV as hex
    subtree:add(f_iv, buffer(offset, IV_SIZE))
    offset = offset + IV_SIZE

    -- Payload (hex)
    local payload_len = total_len - offset - TAG_SIZE
    if payload_len < 0 then payload_len = 0 end
    subtree:add(f_payload, buffer(offset, payload_len))
    offset = offset + payload_len

    -- Tag
    subtree:add(f_tag, buffer(offset, TAG_SIZE))

    return total_len
end

function EASY.dissector(buffer, pinfo, tree)
    return dissect_easy(buffer, pinfo, tree)
end

local udp_port = DissectorTable.get("udp.port")
udp_port:add(54000, EASY)
