with GenericAbstractModel;
procedure GenericAbstractCallErrors is
    package Model renames GenericAbstractModel;
    generic
        type T is tagged private;
        with function Score (Object : T; Extra : Integer := 5) return Integer is abstract;
        with function Construct return T is abstract;
    package Static_Calls is
        function Illegal (Object : T) return Integer;
        function Legal (Object : T'Class) return Integer;
    end Static_Calls;
    package body Static_Calls is
        function Illegal (Object : T) return Integer is
            Local : T := Construct;
        begin
            return Score (Object) + Score (Construct);
        end Illegal;
        function Legal (Object : T'Class) return Integer is
        begin
            return Score (Object);
        end Legal;
    end Static_Calls;
begin
    null;
end GenericAbstractCallErrors;
