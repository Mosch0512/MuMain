// doctest unit tests for the bounds and names of head code 45 entries, the
// transformed players that come into view.
//
// Run: ctest --test-dir <build directory> --build-config Release -R "transform_viewport"

#include "doctest.h"

#include "Network/Server/TransformViewportEntry.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <vector>

using Network::Viewport::TransformViewportEntryLength;
using Network::Viewport::WritePlayerName;

namespace
{
constexpr std::size_t HeaderLength = sizeof(PWHEADER_DEFAULT_WORD);
constexpr std::size_t FixedEntryLength = offsetof(PCREATE_TRANSFORM_EXTENDED, s_BuffEffectState);
constexpr std::size_t BuffCountIndex = offsetof(PCREATE_TRANSFORM_EXTENDED, s_BuffCount);

// A packet with one entry per buff count, each carrying only its own buffs.
std::vector<BYTE> TransformPacket(const std::vector<BYTE>& buffCounts)
{
    std::vector<BYTE> packet(HeaderLength);
    for (const BYTE buffCount : buffCounts)
    {
        std::vector<BYTE> entry(FixedEntryLength + buffCount);
        entry[BuffCountIndex] = buffCount;
        packet.insert(packet.end(), entry.begin(), entry.end());
    }

    return packet;
}

// An entry whose name holds these bytes; the rest of its name stays zero.
PCREATE_TRANSFORM_EXTENDED EntryNamed(std::string_view name)
{
    PCREATE_TRANSFORM_EXTENDED entry{};
    std::memcpy(entry.ID, name.data(), std::min(name.size(), sizeof entry.ID));
    return entry;
}
} // namespace

TEST_CASE("An entry is as long as its fixed fields and the buffs it carries [network][transform_viewport]")
{
    for (const BYTE buffCount : {0, 1, 15, MAX_BUFF_SLOT_INDEX})
    {
        CAPTURE(static_cast<int>(buffCount));
        const std::vector<BYTE> packet = TransformPacket({buffCount});

        const auto length = TransformViewportEntryLength(packet, HeaderLength);

        REQUIRE(length.has_value());
        CHECK(*length == FixedEntryLength + buffCount);
        CHECK(HeaderLength + *length == packet.size());
    }
}

TEST_CASE("Entries follow each other by their lengths [network][transform_viewport]")
{
    const std::vector<BYTE> packet = TransformPacket({2, 0, 5});
    std::size_t offset = HeaderLength;

    for (const BYTE buffCount : {2, 0, 5})
    {
        const auto length = TransformViewportEntryLength(packet, offset);
        REQUIRE(length.has_value());
        CHECK(*length == FixedEntryLength + buffCount);
        offset += *length;
    }

    CHECK(offset == packet.size());
    CHECK_FALSE(TransformViewportEntryLength(packet, offset).has_value());
}

TEST_CASE("A truncated entry is refused [network][transform_viewport]")
{
    const std::vector<BYTE> packet = TransformPacket({3});

    const std::vector<BYTE> withoutLastBuff(packet.begin(), packet.end() - 1);
    CHECK_FALSE(TransformViewportEntryLength(withoutLastBuff, HeaderLength).has_value());

    const std::vector<BYTE> withoutBuffs(packet.begin(), packet.begin() + HeaderLength + FixedEntryLength - 1);
    CHECK_FALSE(TransformViewportEntryLength(withoutBuffs, HeaderLength).has_value());

    CHECK_FALSE(TransformViewportEntryLength(packet, packet.size() + 1).has_value());
}

TEST_CASE("An entry with more buffs than a character can have is refused [network][transform_viewport]")
{
    const std::vector<BYTE> packet = TransformPacket({MAX_BUFF_SLOT_INDEX + 1});

    CHECK_FALSE(TransformViewportEntryLength(packet, HeaderLength).has_value());
}

TEST_CASE("A name of all its bytes replaces the form's name without a tail [network][transform_viewport]")
{
    constexpr std::string_view FullName = "Abcdefghij";
    static_assert(FullName.size() == MAX_USERNAME_SIZE);
    // The transformed player is created as a monster, with the form's name.
    wchar_t name[MAX_MONSTER_NAME + 1] = L"Golden Budge Dragon";

    WritePlayerName(EntryNamed(FullName), name);

    CHECK(std::wstring_view(name) == L"Abcdefghij");
}

TEST_CASE("A shorter name replaces the form's name [network][transform_viewport]")
{
    wchar_t name[MAX_MONSTER_NAME + 1] = L"Golden Budge Dragon";

    WritePlayerName(EntryNamed("J\xC3\xB6rg"), name);

    CHECK(std::wstring_view(name) == L"J\u00F6rg");
}

TEST_CASE("A name is cut to the room it is written to [network][transform_viewport]")
{
    wchar_t name[5] = {L'x', L'x', L'x', L'x', L'x'};

    WritePlayerName(EntryNamed("Abcdefghij"), name);

    REQUIRE(name[4] == L'\0');
    CHECK(std::wstring_view(name) == L"Abcd");
}
