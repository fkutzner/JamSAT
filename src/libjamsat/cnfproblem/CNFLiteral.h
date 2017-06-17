#pragma once

#include <ostream>

namespace jamsat {
    /**
     * \defgroup JamSAT_CNFProblem Collection of CNF-related data types.
     */
    
    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \enum jamsat::CNFSign
     * \brief A sign datatype for CNF literals.
     */
    enum class CNFSign {
        /// The negative literal sign.
        NEGATIVE = 0,
        /// The positive literal sign.
        POSITIVE = 1
    };
    
    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \brief Inverts the given CNF literal sign.
     *
     * \param sign      The sign to be inverted.
     * \returns         The inverted sign.
     */
    inline CNFSign invert(CNFSign sign) {
        int rawSign = static_cast<int>(sign);
        return static_cast<CNFSign>(1-rawSign);
    }
    
    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \class jamsat::CNFVar
     * \brief A CNF variable class.
     */
    class CNFVar {
    public:
        /** The underlying variable type. */
        using RawVariableType = int;
        
        /** 
         * \brief Constructs a CNFVar object.
         *
         * \param variableValue  The non-negative raw variable identifier.
         */
        explicit inline CNFVar(RawVariableType variableValue);
        
        /**
         * \brief Gets the variable's raw value.
         *
         * \returns the variable's raw value.
         */
        inline RawVariableType getRawValue() const noexcept;
        
        inline bool operator==(const CNFVar &rhs) const noexcept;
    private:
        RawVariableType m_value;
    };

    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \class jamsat::CNFLit
     * \brief A CNF literal class.
     */
    class CNFLit {
    public:
        /**
         * \brief Constructs a CNFLit object.
         *
         * \param variable  The literal's variable.
         * \param sign      The literal's sign.
         */
        inline CNFLit(CNFVar variable, CNFSign sign);
        
        /**
         * \brief Gets the literal's variable.
         *
         * \returns  The literal's variable.
         */
        inline CNFVar getVariable() const noexcept;
        
        /**
         * \brief Gets the literal's sign.
         *
         * \returns  The literal's sign.
         */
        inline CNFSign getSign() const noexcept;
        
        /**
         * \brief Gets the literal's negate.
         *
         * \returns The literal's negate.
         */
        inline CNFLit operator~() const noexcept;
        
        inline bool operator==(const CNFLit &rhs) const noexcept;
        inline bool operator!=(const CNFLit &rhs) const noexcept;
    private:
        int m_value;
    };
    
    std::ostream& operator<< (std::ostream& stream, const CNFVar& variable);
    std::ostream& operator<< (std::ostream& stream, const CNFLit& literal);

    /********** Implementation ****************************** */
    
    CNFVar::CNFVar(RawVariableType variableValue)
    : m_value(variableValue) {
    }
    
    CNFVar::RawVariableType CNFVar::getRawValue() const noexcept {
        return m_value;
    }
    
    bool CNFVar::operator==(const CNFVar& rhs) const noexcept {
        return rhs.m_value == m_value;
    }
    
    CNFLit::CNFLit(CNFVar variable, CNFSign sign) {
        m_value = (variable.getRawValue() << 1) | static_cast<int>(sign);
    }
    
    CNFVar CNFLit::getVariable() const noexcept {
        return CNFVar{m_value >> 1};
    }
    
    CNFSign CNFLit::getSign() const noexcept {
        return static_cast<CNFSign>(m_value & 1);
    }

    CNFLit CNFLit::operator~() const noexcept {
        return CNFLit{getVariable(), invert(getSign())};
    }
    
    bool CNFLit::operator==(const CNFLit& rhs) const noexcept {
        return rhs.m_value == m_value;
    }
    
    bool CNFLit::operator!=(const CNFLit& rhs) const noexcept {
        return !(*this == rhs);
    }
}
