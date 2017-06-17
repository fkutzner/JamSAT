#include "CNFLiteral.h"

namespace jamsat {
    std::ostream& operator<< (std::ostream& stream, const CNFVar& variable) {
        stream << variable.getRawValue();
        return stream;
    }
    
    std::ostream& operator<< (std::ostream& stream, const CNFLit& literal) {
        stream << (literal.getSign() == CNFSign::POSITIVE ? "+" : "-");
        stream << literal.getVariable();
        return stream;
    }
}
