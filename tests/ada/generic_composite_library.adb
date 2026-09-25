with Ada.Text_IO;
package Composite_State is
    type Vector is array (Integer range <>) of Integer;
    type Pair is record
        Value : Integer;
    end record;
    Data : Vector (7 .. 9) := (1, 2, 3);
    Item : Pair := (Value => 5);
    Calls : Integer := 0;
    function Make return Pair;
end Composite_State;
package body Composite_State is
    function Make return Pair is
    begin
        Calls := Calls + 1;
        return Item;
    end Make;
end Composite_State;
generic
    Initial : Composite_State.Vector;
    Target : in out Composite_State.Vector;
    Item : Composite_State.Pair := Composite_State.Make;
package Composite_Template is
    function First return Integer;
    function Value return Integer;
    procedure Restore;
end Composite_Template;
package body Composite_Template is
    function First return Integer is
    begin
        return Initial'First;
    end First;
    function Value return Integer is
    begin
        return Item.Value;
    end Value;
    procedure Restore is
    begin
        Target := Initial;
    end Restore;
begin
    Target (Target'First) := 9;
end Composite_Template;
package Composite_First is new Composite_Template (Composite_State.Data, Composite_State.Data);
package Composite_Second is new Composite_Template (Composite_State.Data, Composite_State.Data);
procedure Generic_Composite_Library is
begin
    Composite_State.Item.Value := 20;
    Composite_First.Restore;
    if Composite_State.Data (7) /= 1 or Composite_First.First /= 7
        or Composite_First.Value /= 5 or Composite_Second.Value /= 5 then
        raise Program_Error;
    end if;
    Composite_Second.Restore;
    if Composite_State.Data (7) /= 9 or Composite_State.Calls /= 2 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("library composite generic copies, defaults, references, and order");
end Generic_Composite_Library;
