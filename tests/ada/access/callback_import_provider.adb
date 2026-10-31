package body Callback_Import_Provider is
    function Increment (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Increment;
    function Echo (Value : String) return String is
    begin
        return Value;
    end Echo;
    function Fail (Value : Integer) return Integer is
    begin
        raise Constraint_Error;
    end Fail;
end Callback_Import_Provider;
