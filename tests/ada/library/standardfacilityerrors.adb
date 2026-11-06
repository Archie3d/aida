with System;
procedure StandardFacilityErrors is
    function Bad_Result (Left, Right : System.Address) return Integer renames System."<";
    function Bad_Mode (Left : out System.Address; Right : System.Address) return Boolean renames System."=";
    generic
        with function Compare (Left, Right : System.Address) return Integer;
    procedure Generic_Test;
    procedure Generic_Test is
    begin
        null;
    end Generic_Test;
    procedure Bad_Actual is new Generic_Test (System."<");
    function Less (Left, Right : System.Address) return Boolean renames System."<";
    type Comparison is access function (Left, Right : System.Address) return Boolean;
    Pointer : Comparison := Less'Access;
    type Object_Pointer is access all Integer;
    Invalid : Object_Pointer := Object_Pointer (System.Null_Address);
    Value : Integer := ASCII.HT;
    Name : System.Name := System.Low_Order_First;
begin
    System.Default_Priority := 1;
    ASCII.NUL := 'X';
end StandardFacilityErrors;
