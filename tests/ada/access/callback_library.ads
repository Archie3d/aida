package Callback_Library is
    type Function_Access is access function (X : Integer) return Integer;
    type Procedure_Access is access procedure (X : out Integer);
    function Double (X : Integer) return Integer;
    procedure Set_Value (X : out Integer);
    function Choose return Function_Access;
end Callback_Library;
