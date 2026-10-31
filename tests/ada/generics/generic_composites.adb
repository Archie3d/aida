with Ada.Text_IO;
procedure Generic_Composites is
    type Vector is array (Integer range <>) of Integer;
    subtype Triple is Vector (1 .. 3);
    type Pair is record
        Value : Integer;
        Data : Triple;
    end record;
    generic
        Initial : Pair;
        Target : in out Pair;
        Default_Value : Pair := Initial;
    package Records is
        function Read return Pair;
        procedure Restore;
    end Records;
    package body Records is
        function Read return Pair is
        begin
            return Default_Value;
        end Read;
        procedure Restore is
        begin
            Target := Initial;
            Target.Value := Target.Value + 1;
            Target.Data (2) := 99;
        end Restore;
    end Records;
    generic
        Initial : Triple;
        Target : in out Triple;
    package Arrays is
        function Read return Triple;
        procedure Restore;
    end Arrays;
    package body Arrays is
        function Read return Triple is
        begin
            return Initial;
        end Read;
        procedure Restore is
        begin
            Target := Initial;
            Target (Target'First) := 42;
        end Restore;
    end Arrays;
    Calls : Integer := 0;
    function Make return Pair is
    begin
        Calls := Calls + 1;
        return (Value => Calls, Data => (1, 2, 3));
    end Make;
    A : Pair := (Value => 5, Data => (4, 5, 6));
    B : Pair := (Value => 8, Data => (7, 8, 9));
    package First is new Records (A, B);
    package Second is new Records (Make, A);
    Source : Vector (7 .. 9) := (10, 20, 30);
    Target : Vector (11 .. 13) := (0, 0, 0);
    package Shifted is new Arrays (Source, Target);
    package Alias is new Arrays (Target, Target);
    type Record_Array is array (1 .. 2) of Pair;
    Items : Record_Array := (others => (Value => 0, Data => (0, 0, 0)));
    Index : Integer := 1;
    package Selected is new Records (A, Items (Index));
    generic
        type Item is private;
        Value : Item;
        Other : Item := Value;
    function Copy return Item;
    function Copy return Item is
    begin
        return Other;
    end Copy;
    function Saved is new Copy (Pair, A);
    Result : Pair;
begin
    A.Value := 100;
    A.Data (1) := 100;
    Source (7) := -1;
    Index := 2;
    First.Restore;
    if B.Value /= 6 or B.Data /= (4, 99, 6) then
        raise Program_Error;
    end if;
    Result := First.Read;
    if Result.Value /= 5 or Result.Data /= (4, 5, 6) then
        raise Program_Error;
    end if;
    Second.Restore;
    if A.Value /= 2 or A.Data /= (1, 99, 3) or Calls /= 1 then
        raise Program_Error;
    end if;
    Selected.Restore;
    Result := Saved;
    if Items (1).Value /= 6 or Items (2).Value /= 0 or Result.Value /= 5 then
        raise Program_Error;
    end if;
    Shifted.Restore;
    if Target /= (42, 20, 30) or Shifted.Read /= (10, 20, 30) then
        raise Program_Error;
    end if;
    Alias.Restore;
    if Target /= (42, 0, 0) then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic arrays and records: copies, defaults, aliases, and sliding");
end Generic_Composites;
