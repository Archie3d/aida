with Ada.Text_IO; use Ada.Text_IO;
procedure Callbackrenames is
    type Callback is access function (X : Integer) return Integer;
    type Action is access procedure;
    Count : Integer := 0;
    function Add (X : Integer) return Integer is
    begin
        return X + 10;
    end Add;
    function Other (X : Integer) return Integer is
    begin
        return X + 20;
    end Other;
    procedure Tick is
    begin
        Count := Count + 1;
    end Tick;
    F : Callback := Add'Access;
    P : Action := Tick'Access;
    function Choose return Callback is
    begin
        Count := Count + 1;
        return F;
    end Choose;
    function Saved (Value : Integer := 2) return Integer renames Choose.all;
    function Completed (X : Integer) return Integer;
    function Completed (X : Integer) return Integer renames F.all;
    procedure Saved_Tick renames P.all;
    G : Callback := Saved'Access;
    function Copy return Callback is
        function Local (X : Integer) return Integer renames F.all;
    begin
        return Local'Access;
    end Copy;
    Copied : Callback := Copy;
    procedure Nested is
        function Chain (X : Integer) return Integer renames Saved;
    begin
        Put_Line (Integer'Image (Chain (3)));
    end Nested;
begin
    F := Other'Access;
    P := null;
    Put_Line (Integer'Image (Saved));
    Put_Line (Integer'Image (Completed (3)));
    Nested;
    Saved_Tick;
    Put_Line (Integer'Image (Count));
    if G = Add'Access and Copied = G then
        Put_Line ("saved identity");
    end if;
    begin
        declare
            procedure Null_Rename renames P.all;
        begin
            Put_Line ("unexpected");
        end;
    exception
        when Constraint_Error => Put_Line ("null at elaboration");
    end;
end Callbackrenames;
