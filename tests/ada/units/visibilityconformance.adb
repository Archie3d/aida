with Ada.Text_IO; use Ada.Text_IO;
procedure VisibilityConformance is
    subtype Small is Integer range 1 .. 10;
    subtype Same is Integer range 1 .. 10;
    package Defaults is
        Value : constant := 4;
    end Defaults;
    use Defaults;
    function Pick (X : Small := Value) return Small;
    function Pick (X : Same := Defaults.Value) return Same is
    begin
        return X;
    end Pick;
    function Pick (X : Boolean) return Integer is
    begin
        if X then return 8; end if;
        return 9;
    end Pick;
    procedure Omitted (X : Integer := 3);
    procedure Omitted (X : Integer) is
    begin
        if X /= 3 then raise Program_Error; end if;
    end Omitted;
    package P is
        Value : constant := 99;
        function Get return Integer;
        type Hidden is private;
        function Make return Hidden;
    private
        type Hidden is new Integer;
        Secret : constant := 6;
    end P;
    package body P is
        function Get return Integer is begin return Secret; end Get;
        function Make return Hidden is begin return Hidden (Secret); end Make;
    end P;
    Direct : constant Standard.Integer := 7;
    package Q is
        Direct : constant := 99;
    end Q;
    procedure Check is
        use Q;
    begin
        if Direct /= 7 then raise Program_Error; end if;
    end Check;
    procedure Nested is
        package P is
            Value : constant := 2;
        end P;
    begin
        if P.Value /= 2 then raise Program_Error; end if;
    end Nested;
    Object : P.Hidden := P.Make;
begin
    if Pick /= 4 or Pick (True) /= 8 or P.Get /= 6 then
        raise Program_Error;
    end if;
    Omitted;
    Check;
    Nested;
    Put_Line ("visibility and profiles checked");
end VisibilityConformance;
