#include "AST.hh"

#define VISIT(T) ErrorInfo operator()(const T &arg) const  { \
    return arg.info; \
}
#define VISITP(T) ErrorInfo operator()(const std::unique_ptr<T> &arg) const { \
    return arg->info; \
}

namespace Kaleidoscope {
namespace AST {


struct InfoVisitor: public boost::static_visitor<ErrorInfo> {
    VISIT(NumberLiteral)
    VISIT(VariableName)

    VISITP(BinaryOp)
    VISITP(FunctionCall)
    VISITP(IfThenElse)
    VISITP(ForLoop)
    VISITP(LocalVar)
};

static const InfoVisitor visitor = InfoVisitor();

ErrorInfo get_info(const Expression &expr) {
    return boost::apply_visitor(visitor, expr);
}


}
}
