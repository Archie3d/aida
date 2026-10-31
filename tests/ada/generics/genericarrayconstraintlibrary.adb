package body GenericArrayConstraintLibrary is
    function Copy (Value : Vector) return Vector is
        Result : Vector := Value;
    begin
        return Result;
    end Copy;
    function Copy_Open (Value : Open_Vector) return Open_Vector is
    begin
        return Value;
    end Copy_Open;
    package body Inner is
        function Copy (Value : Other) return Other is
        begin
            return Value;
        end Copy;
    end Inner;
end GenericArrayConstraintLibrary;
