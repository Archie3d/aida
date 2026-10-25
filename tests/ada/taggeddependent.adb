with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure TaggedDependent is
    procedure Run (Count : Positive) is
        package Model is
            type Root (Length : Positive) is tagged record
                Text : String (1 .. Length) := (others => 'a');
                Value : Integer := 5;
            end record;
            function Read (Self : Root) return Integer;
            type Renamed (Count : Positive) is new Root (Count) with record
                More : String (1 .. Count) := (others => 'r');
            end record;
            type Same (Length : Positive) is new Root (Length) with null record;
            type Fixed is new Root (3) with null record;
            type Child is new Root with record
                Extra : String (1 .. Length) := (others => 'b');
            end record;
        end Model;
        package body Model is
            function Read (Self : Root) return Integer is
            begin
                return Self.Text'Length + Self.Value;
            end Read;
        end Model;
        Renamed : Model.Renamed (Count);
        Same : Model.Same (Count);
        Fixed : Model.Fixed;
        Aggregate : Model.Renamed := (Count, (others => 'g'), 8, (others => 'h'));
        Extended : Model.Renamed := (Model.Root with Count => Count, More => (others => 'e'));
        Item : Model.Child (Count);
        Other : Model.Child (Count);
        Wide : Model.Root'Class := Item;
        Copy : Model.Root'Class := Wide;
        type Link is access Model.Root'Class;
        Pointer : Link := new Model.Child (Count);
        procedure Free is new Ada.Unchecked_Deallocation (Model.Root'Class, Link);
    begin
        if Same.Length /= Count or else Same.Text'Length /= Count or else Renamed.Count /= Count or else Model.Root (Renamed).Length /= Count
            or else Renamed.Text'Length /= Count or else Renamed.More (Count) /= 'r'
            or else Fixed.Text'Length /= 3 or else Aggregate.Text (Count) /= 'g'
            or else Aggregate.More (Count) /= 'h' or else Aggregate.Value /= 8
            or else Extended.Text'Length /= Count or else Extended.More (Count) /= 'e' then
            raise Program_Error;
        end if;
        Item.Text (Count) := 'z';
        Model.Root (Item) := Model.Root (Other);
        Item.Extra (Count) := 'q';
        if Item.Text (Count) /= 'a' or else Other.Extra (Count) /= 'b'
            or else Model.Read (Wide) /= Count + 5 then
            raise Program_Error;
        end if;
        Model.Child (Wide).Text (Count) := 'w';
        if Model.Child (Copy).Text (Count) /= 'a' then
            raise Program_Error;
        end if;
        Copy := Wide;
        if Model.Child (Copy).Text (Count) /= 'w' then
            raise Program_Error;
        end if;
        Free (Pointer);
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (11);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("tagged dependent worker failed");
    end Worker;
begin
    Run (7);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged dependent storage passed");
end TaggedDependent;
