with Ada.Text_IO;
procedure PrivateDelayed is
    procedure Run (Count : Positive) is
        package Model is
            type Hidden is private;
            type Root is tagged record
                Data : Hidden;
                Tail : Integer := 13;
            end record;
            function Read (Self : Root) return Integer;
            type Wrapper is record
                Item : Root;
                Value : Integer := 17;
            end record;
        private
            type Hidden is record
                Text : String (1 .. Count) := (others => 'x');
                Value : Integer := 5;
            end record;
        end Model;
        package body Model is
            function Read (Self : Root) return Integer is
            begin
                return Self.Data.Text'Length + Self.Data.Value + Self.Tail;
            end Read;
        end Model;
        Item : Model.Wrapper;
        Wide : Model.Root'Class := Item.Item;
    begin
        if Model.Read (Wide) /= Count + 18 or else Item.Value /= 17 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (19);
        Run (3);
    exception
        when others => Ada.Text_IO.Put_Line ("private delayed worker failed");
    end Worker;
begin
    Run (7);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("private delayed layouts passed");
end PrivateDelayed;
