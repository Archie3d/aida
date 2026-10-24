with Unchecked_Model; use Unchecked_Model;
procedure UncheckedAccessErrors is
    Plain : Integer := 1;
    Object : aliased Integer := 2;
    Const : aliased constant Integer := 3;
    Positive_Object : aliased Positive := 4;
    type Pool is access Integer;
    type Callback is access procedure;
    procedure Action is
    begin
        null;
    end Action;
    function Value return Integer is
    begin
        return 5;
    end Value;
    A : Integer_Link := Plain'Unchecked_Access;
    B : Integer_Link := Const'Unchecked_Access;
    C : Integer_Link := Positive_Object'Unchecked_Access;
    D : Pool := Object'Unchecked_Access;
    E : Callback := Action'Unchecked_Access;
    F : Integer := Object'Unchecked_Access;
    G : Integer_Link := Integer'Unchecked_Access;
    H : Integer_Link := Value'Unchecked_Access;
    I : Integer_Link := Object'Unchecked_Access (1);
    Variant_Object : aliased Mutable;
    K : Integer_Link := Variant_Object.Value'Unchecked_Access;
    type Elements is array (1 .. 2) of Integer;
    Items : Elements := (1, 2);
    L : Integer_Link := Items (1)'Unchecked_Access;
    type Small is record
        Field : Integer;
    end record;
    subtype Padded is Small;
    for Padded'Size use 64;
    Small_Object : aliased Small;
    type Large_Link is access all Padded;
    M : Large_Link := Small_Object'Unchecked_Access;
begin
    declare
        Local : aliased Integer := 7;
        Checked : Integer_Link := Local'Access;
    begin
        null;
    end;
end UncheckedAccessErrors;
