#pragma once

#include <string>
#include <vector>

struct Expr;
struct Type;

// Helpers shared by the semantic analyzer implementation.
namespace SemaSupport
{

int objectView(Expr* expr);
bool conformingExpressions(const Expr* left, const Expr* right);
bool staticallyMatches(Type* actual, Type* formal);
bool isUniversal(const Type* type);
void adaptUniversal(Expr* expr, Type* type);
std::vector<std::string> splitDottedName(const std::string& name);

}
