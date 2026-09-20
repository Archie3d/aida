#include "Ast.h"

BlockStmt::BlockStmt()
    : Stmt(StmtKind::Block)
{
}

BlockStmt::~BlockStmt() = default;

const char* operatorName(BinaryOp op)
{
    switch (op) {
    case BinaryOp::Add: return "+";
    case BinaryOp::Subtract: return "-";
    case BinaryOp::Multiply: return "*";
    case BinaryOp::Divide: return "/";
    case BinaryOp::Modulo: return "mod";
    case BinaryOp::Remainder: return "rem";
    case BinaryOp::Power: return "**";
    case BinaryOp::Concatenate: return "&";
    case BinaryOp::And: return "and";
    case BinaryOp::Or: return "or";
    case BinaryOp::Xor: return "xor";
    case BinaryOp::Equal: return "=";
    case BinaryOp::NotEqual: return "/=";
    case BinaryOp::Less: return "<";
    case BinaryOp::LessEqual: return "<=";
    case BinaryOp::Greater: return ">";
    case BinaryOp::GreaterEqual: return ">=";
    case BinaryOp::AndThen:
    case BinaryOp::OrElse: return "";
    }
    return "";
}

const char* operatorName(UnaryOp op)
{
    switch (op) {
    case UnaryOp::Plus: return "+";
    case UnaryOp::Negate: return "-";
    case UnaryOp::Not: return "not";
    case UnaryOp::Abs: return "abs";
    }
    return "";
}

std::string operatorSymbol(const std::string& spelling)
{
    std::string name = spelling;
    for (char& c : name) {
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
    }
    for (const char* symbol : { "+", "-", "*", "/", "mod", "rem", "**", "&", "and", "or", "xor",
                               "=", "/=", "<", "<=", ">", ">=", "abs", "not" }) {
        if (name == symbol) {
            return name;
        }
    }
    return "";
}
