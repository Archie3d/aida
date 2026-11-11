#pragma once

#include <string>
#include <vector>

struct Expr;
struct Type;

// Helpers shared by the semantic analyzer implementation.
namespace SemaSupport {

enum class ObjectView
{
    NotObject,
    Constant,
    Variable
};

// Classifies names for renaming, assignment and access-attribute legality.
ObjectView objectView(Expr* expr);
bool conformingExpressions(const Expr* left, const Expr* right);
bool staticallyMatches(Type* actual, Type* formal);
bool profileTypeMatches(Type* actual, Type* formal);
std::string expectedActualTypes(const Type* expected, const Type* actual);
bool isUniversal(const Type* type);
void adaptUniversal(Expr* expr, Type* type);
std::vector<std::string> splitDottedName(const std::string& name);

} // namespace SemaSupport
