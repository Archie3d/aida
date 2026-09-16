#pragma once

#include <string>
#include <vector>

// Remembers what each object file was built from, so that a library unit whose
// intermediate language has not changed keeps the object it already has.
class Cache
{
public:
    Cache(std::string directory, std::string stamp);

    // True when the object for this unit is there and was built from the
    // intermediate language now sitting beside it.
    bool isCurrent(const std::string& unit) const;

    // Called once the object has been rebuilt.
    void record(const std::string& unit) const;

private:
    std::string m_directory;
    std::string m_stamp;
};

// A 64 bit FNV-1a digest written as hexadecimal.  Content decides what is out
// of date, since a rewritten file with the same text is no change at all.
std::string digestOf(const std::string& text);
std::string digestOfFile(const std::string& path);

// Identifies the programs that turn intermediate language into objects, so
// that replacing one of them is not mistaken for nothing having happened.
std::string toolDigest(const std::vector<std::string>& programs);
