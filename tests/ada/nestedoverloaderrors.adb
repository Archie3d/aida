procedure NestedOverloadErrors is
    function Pick return Integer is
    begin
        return 1;
    end Pick;
    function Pick return Boolean is
    begin
        return True;
    end Pick;
    procedure Consume (Value : Integer; Flag : Boolean := True) is
    begin
        null;
    end Consume;
    procedure Consume (Value : Boolean; Flag : Integer := 1) is
    begin
        null;
    end Consume;
    function Wrap (Value : Integer) return Integer is
    begin
        return Value;
    end Wrap;
    function Wrap (Value : Boolean) return Boolean is
    begin
        return Value;
    end Wrap;
    function Same_Result (Value : Integer) return Integer is
    begin
        return Value;
    end Same_Result;
    function Same_Result (Value : Boolean) return Integer is
    begin
        return 0;
    end Same_Result;
    X : Integer;
begin
    Consume (Pick);
    Consume (Wrap (Pick));
    Consume (Pick, 'x');
    Consume (Value => Pick, Missing => True);
    X := Same_Result (Pick);
    X := Wrap (True);
end NestedOverloadErrors;
