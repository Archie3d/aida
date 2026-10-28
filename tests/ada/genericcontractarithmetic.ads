with GenericContractEnvironment; use GenericContractEnvironment;
generic
    type Number is range <>;
package GenericContractArithmetic is
    generic
        N : Number;
    function Sum return Number;
end GenericContractArithmetic;
