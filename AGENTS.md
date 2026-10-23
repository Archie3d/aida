---
description: Ada compiler with QBE backend
globs: "**/*"
alwaysApply: false
---

# Code style
- Use 4 spaces indentation (for C, C++, and Ada code)
- Functions code open with curly brace on the a line:
```cpp
void func()
{
    // code
}
```

- Classes and struct definition curly braces start on a new line:
```cpp
class Compiler
{
public:
    Compiler();

protected:

private:
    std::string m_errorMessage;
};
```

In classes the sections should be in this order: `public`, `protected`, `private`.

- C++ Class member variables have `m_` prefix, but public structs in C++ and C should not:

```cpp
class Data
{
public:
    struct Item
    {
        uint64_t id{};
        std::string name{};
    };
private:
    std::vector<Item> m_items{};
};
```

- Inline getters and setters may stay on one line if reasonably short:
```cpp
class Record
{
public:
    int getId() const { return m_id; }
    void setId(int newId) { m_id = newId; }

private:
    int m_id{};
};
```

- Constants must have `k` prefix, for example:
```cpp
struct Object
{
    static const size_t kBufferSize = 1024;
};

constexpr float kFactor = 0.1f;
```

- Preprocessor definitions must be capitalised and use snake-case style:
```c
#define USE_DIAGNOSTICS 1
#define ADJUST_VALUE(x) ((x) * 2)
```

- C++ should prefer `constexpr` constants and templated types, but macros are still allowed when sensible.

- Identifiers use Camel case convention, with UpperCamelCase for types, and lowerCamelCase for variables.

- All other code scopes have curly brase put at the end of the statement line:
```cpp
if (condition) {
    doSomething();
    while (anotherCondition) {
        doSomethingElse();
    }
}
```

- Ada runtime C exports must use `__ada_` prefix:
```c
double __ada_numerics_sqrt(double x);
```

# Instructions
- `qbe` folder is a QBE backend - an external project, you can read it but you must not modify it.
- Start with a simple top level Ada program definitions.
- Gradually add support for more complex statements, data types, functions, procedures.
- Add simle test Ada programs to test the compiler development stages.
- Follow the roadmap in the order described in TODO.md, Mark completed steps as `[x]`.
- Unsupported constructs should produce diagnostics rather than be silently ignored.
- Keep README.md concise. It should remain brief and easily readable. Track the progress via TODO.md file.
