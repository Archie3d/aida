procedure CaseSelectorErrors is
    type Left_Number is range 0 .. 3;
    type Right_Number is range 0 .. 3;
    type Color is (Red, Blue);
    function Two_Numbers return Left_Number is
    begin
        return 1;
    end Two_Numbers;
    function Two_Numbers return Right_Number is
    begin
        return 1;
    end Two_Numbers;
    function Number_Or_Color return Integer is
    begin
        return 1;
    end Number_Or_Color;
    function Number_Or_Color return Color is
    begin
        return Red;
    end Number_Or_Color;
    function Same_Result (Value : Left_Number) return Integer is
    begin
        return 1;
    end Same_Result;
    function Same_Result (Value : Right_Number) return Integer is
    begin
        return 1;
    end Same_Result;
    function Not_Discrete return Float is
    begin
        return 1.0;
    end Not_Discrete;
    function Not_Discrete return String is
    begin
        return "one";
    end Not_Discrete;
    function Real_Value return Float is
    begin
        return 1.0;
    end Real_Value;
    package Hidden is
        type Number is private;
        function Value return Number;
    private
        type Number is new Integer;
    end Hidden;
    package body Hidden is
        function Value return Number is
        begin
            return 1;
        end Value;
    end Hidden;
    function Choose return Integer is
    begin
        return 1;
    end Choose;
    generic
        type Scalar is range <>;
        with function Choose return Scalar;
    procedure Generic_Ambiguous;
    procedure Generic_Ambiguous is
    begin
        case Choose is
            when others => null;
        end case;
    end Generic_Ambiguous;
    generic
        type Scalar is private;
    procedure Generic_Private (Value : Scalar);
    procedure Generic_Private (Value : Scalar) is
    begin
        case Value is
            when others => null;
        end case;
    end Generic_Private;
begin
    case Two_Numbers is
        when Left_Number'(1) => null;
        when others => null;
    end case;
    -- Choices supply no context to resolve the selector.
    case Number_Or_Color is
        when Red => null;
        when Blue => null;
    end case;
    -- A unique result type does not resolve ambiguous argument profiles.
    case Same_Result (1) is
        when others => null;
    end case;
    case Not_Discrete is
        when others => null;
    end case;
    case Real_Value is
        when others => null;
    end case;
    case Hidden.Value is
        when others => null;
    end case;
    case 1.0 is
        when others => null;
    end case;
end CaseSelectorErrors;
