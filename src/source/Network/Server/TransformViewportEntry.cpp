#include "stdafx.h"

#include "Network/Server/TransformViewportEntry.h"

#include "Core/Text/WideString.h"

namespace Network::Viewport
{
std::optional<std::size_t> TransformViewportEntryLength(std::span<const BYTE> packet, std::size_t offset)
{
    constexpr std::size_t FixedLength = offsetof(PCREATE_TRANSFORM_EXTENDED, s_BuffEffectState);
    if (offset > packet.size() || packet.size() - offset < FixedLength)
    {
        return std::nullopt;
    }

    const auto* entry = reinterpret_cast<const PCREATE_TRANSFORM_EXTENDED*>(packet.data() + offset);
    if (entry->s_BuffCount > MAX_BUFF_SLOT_INDEX)
    {
        return std::nullopt;
    }

    const std::size_t length = FixedLength + entry->s_BuffCount;
    if (packet.size() - offset < length)
    {
        return std::nullopt;
    }

    return length;
}

void WritePlayerName(const PCREATE_TRANSFORM_EXTENDED& entry, std::span<wchar_t> name)
{
    wchar_t playerName[MAX_USERNAME_SIZE + 1]{};
    CMultiLanguage::ConvertFromUtf8(playerName, entry.ID, MAX_USERNAME_SIZE);
    Core::Text::CopyWideString(name.data(), name.size(), playerName);
}
} // namespace Network::Viewport
