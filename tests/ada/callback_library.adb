package body Callback_Library is
    function Double (X : Integer) return Integer is
    begin
        return X * 2;
    end Double;
    procedure Set_Value (X : out Integer) is
    begin
        X := 42;
    end Set_Value;
    function Choose return Function_Access is
    begin
        return Double'Access;
    end Choose;
end Callback_Library;
