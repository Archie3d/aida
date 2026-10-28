procedure PragmaArgumentErrors is
    pragma Assert (1);
    pragma Assert (True, 123);
    pragma Assert (Message => "missing condition");
    pragma Assert (Check => True, Check => False);
    pragma Assert (Other => True);
    pragma Assert (Message => "wrong order", Check => True);
    pragma Assert (Check => True, False);
    function F return Integer is
    begin
        return 1;
    end F;
    pragma No_Return (F);
    Value : Integer := 0;
    pragma No_Return (Value);
    procedure Null_Procedure is null;
    pragma No_Return (Null_Procedure);
    generic
    procedure G;
    procedure G is
    begin
        null;
    end G;
    procedure Instance is new G;
    pragma No_Return (Instance);
begin
    null;
end PragmaArgumentErrors;
