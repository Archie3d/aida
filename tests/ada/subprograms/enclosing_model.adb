package body Enclosing_Model is
    Body_Value : Integer := 7;
    function Overload (Value : Boolean) return Integer is
    begin
        return 7;
    end Overload;
    function Overload (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Overload;
    procedure Verify is
        Body_Value : Integer := 99;
    begin
        if Enclosing_Model.Body_Value /= 7 or else Verify.Body_Value /= 99
            or else Enclosing_Model."+" (Number'(2), 1) /= 3 then
            raise Program_Error;
        end if;
        if Enclosing_Model.Overload (1) /= 2 or else Enclosing_Model.Overload (True) /= 7 then
            raise Program_Error;
        end if;
    end Verify;
end Enclosing_Model;
