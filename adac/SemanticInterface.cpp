#include "SemanticInterface.h"

#include "Digest.h"
#include "../support/Artifacts.h"

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

constexpr std::size_t kMaxInterfaceBytes = 64 * 1024 * 1024;
constexpr std::size_t kMaxTokens = 1000000;
constexpr std::size_t kMaxSources = 4096;
constexpr const char* kHeader = "AIDA semantic interface 1\n";

void putInteger(std::ostream& out, uint64_t value)
{
    for (int i = 0; i < 8; ++i) { out.put(static_cast<char>(value >> (i * 8))); }
}

void putString(std::ostream& out, const std::string& value)
{
    putInteger(out, value.size());
    out.write(value.data(), value.size());
}

class Reader
{
public:
    explicit Reader(const std::string& data) : m_data(data) {}

    uint64_t integer()
    {
        if (m_data.size() - m_position < 8) { m_valid = false; return 0; }
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result |= static_cast<uint64_t>(static_cast<unsigned char>(m_data[m_position++])) << (i * 8);
        }
        return result;
    }

    std::string string()
    {
        uint64_t size = integer();
        if (!m_valid || size > m_data.size() - m_position) { m_valid = false; return {}; }
        std::string result = m_data.substr(m_position, size);
        m_position += size;
        return result;
    }

    bool valid() const { return m_valid; }
    bool finished() const { return m_valid && m_position == m_data.size(); }

private:
    const std::string& m_data;
    std::size_t m_position = 0;
    bool m_valid = true;
};

} // namespace

bool writeSemanticInterface(const std::string& path, const SemanticInterface& semanticInterface)
{
    if (semanticInterface.sources.empty() || semanticInterface.sources.size() > kMaxSources) { return false; }
    std::ostringstream payload(std::ios::binary);
    putString(payload, semanticInterface.compilerDigest);
    putString(payload, Artifacts::targetIdentity());
    std::ostringstream metadata;
    writeManifest(metadata, { semanticInterface.unit }, {});
    putString(payload, metadata.str());
    putInteger(payload, semanticInterface.sources.size());
    std::size_t totalTokens = 0;
    for (const auto& source : semanticInterface.sources) {
        if (source.tokens.size() > kMaxTokens - totalTokens) { return false; }
        totalTokens += source.tokens.size();
        putString(payload, source.name);
        putInteger(payload, source.tokens.size());
        for (const Token& token : source.tokens) {
            if (token.subunit != nullptr) { return false; }
            putInteger(payload, static_cast<uint64_t>(token.kind));
            putInteger(payload, token.location.line);
            putInteger(payload, token.location.column);
            putString(payload, token.text);
            putString(payload, token.lower);
            putInteger(payload, std::bit_cast<uint64_t>(static_cast<int64_t>(token.intValue)));
            putInteger(payload, std::bit_cast<uint64_t>(token.realValue));
        }
    }
    std::string data = payload.str();
    if (data.size() > kMaxInterfaceBytes) { return false; }
    Artifacts::AtomicFile publication(path, std::ios::binary);
    auto& out = publication.stream();
    out << kHeader << digestOf(data) << '\n';
    out.write(data.data(), data.size());
    return publication.commit();
}

bool readSemanticInterface(const std::string& path, const std::string& compilerDigest,
    SemanticInterface& semanticInterface, std::string& error)
{
    error = "invalid or truncated semantic interface";
    std::error_code fileError;
    auto size = std::filesystem::file_size(Artifacts::nativePath(path), fileError);
    if (fileError || size > kMaxInterfaceBytes + 128) { return false; }
    std::ifstream in(Artifacts::nativePath(path), std::ios::binary);
    std::string header;
    std::string checksum;
    if (!std::getline(in, header) || header + "\n" != kHeader) {
        error = "unsupported semantic interface format";
        return false;
    }
    if (!std::getline(in, checksum)) { return false; }
    std::ostringstream contents;
    contents << in.rdbuf();
    std::string data = contents.str();
    if (in.bad() || checksum != digestOf(data)) { return false; }
    Reader reader(data);
    SemanticInterface result;
    result.compilerDigest = reader.string();
    if (result.compilerDigest != compilerDigest) {
        error = "semantic interface was produced by a different compiler";
        return false;
    }
    if (reader.string() != Artifacts::targetIdentity()) {
        error = "semantic interface was produced for a different target ABI";
        return false;
    }
    std::istringstream metadata(reader.string());
    std::vector<UnitRecord> units;
    if (!readManifest(metadata, units) || units.size() != 1 || !units.front().interfaceDirectory.empty()) { return false; }
    result.unit = std::move(units.front());
    uint64_t sources = reader.integer();
    if (sources == 0 || sources > kMaxSources) { return false; }
    std::size_t totalTokens = 0;
    for (uint64_t i = 0; i < sources && reader.valid(); ++i) {
        InterfaceSource source;
        source.name = reader.string();
        uint64_t count = reader.integer();
        if (source.name.empty() || source.name.find('\0') != std::string::npos
            || count == 0 || count > kMaxTokens - totalTokens) { return false; }
        totalTokens += count;
        for (uint64_t j = 0; j < count && reader.valid(); ++j) {
            Token token;
            auto kind = reader.integer();
            auto line = reader.integer();
            auto column = reader.integer();
            if (kind > static_cast<uint64_t>(TokenKind::Box) || line > INT32_MAX || column > INT32_MAX) { return false; }
            token.kind = static_cast<TokenKind>(kind);
            token.location.line = static_cast<int>(line);
            token.location.column = static_cast<int>(column);
            token.text = reader.string();
            token.lower = reader.string();
            token.intValue = std::bit_cast<int64_t>(reader.integer());
            token.realValue = std::bit_cast<double>(reader.integer());
            if ((j + 1 == count) != (token.kind == TokenKind::EndOfFile)) { return false; }
            source.tokens.push_back(std::move(token));
        }
        result.sources.push_back(std::move(source));
    }
    if (!reader.finished()) { return false; }
    result.artifactDigest = digestOf(header + "\n" + checksum + "\n" + data);
    semanticInterface = std::move(result);
    return true;
}
