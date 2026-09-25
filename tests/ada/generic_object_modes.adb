procedure Generic_Object_Modes is
    generic
        Value : out Integer;
    procedure Bad_Mode;
    generic
        Value : in out Integer := 1;
    procedure Bad_Default;
begin
    null;
end Generic_Object_Modes;
