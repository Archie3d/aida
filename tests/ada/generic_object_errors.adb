procedure Generic_Object_Errors is
    generic
        Value : Integer;
    procedure Bad_Body;
    procedure Bad_Body is
    begin
        Value := 0;
    end Bad_Body;
    generic
        Value : in out Integer;
    procedure Change;
    procedure Change is
    begin
        Value := Value + 1;
    end Change;
    generic
        Value : Integer;
    package Read_Only is
    end Read_Only;
    Constant_Value : constant Integer := 2;
    function Get return Integer is
    begin
        return 2;
    end Get;
    type Numbers is array (1 .. 2) of Integer;
    Constants : constant Numbers := (1, 2);
    procedure Literal is new Change (1);
    procedure Constant_Actual is new Change (Constant_Value);
    procedure Function_Actual is new Change (Get);
    procedure Component_Actual is new Change (Constants (1));
    package Wrong_Type is new Read_Only (True);
    Wrong : Float := 2.0;
    procedure Wrong_Reference is new Change (Wrong);
    generic
        Value : Numbers;
    package Composite is
    end Composite;
    package Unsupported is new Composite (Constants);
begin
    null;
end Generic_Object_Errors;
