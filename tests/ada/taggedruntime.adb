with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Ada.Finalization;
procedure TaggedRuntime is
    procedure Run (Count : Positive) is
        Live : Integer := 0;
        package Model is
            type Guard is new Ada.Finalization.Controlled with null record;
            overriding procedure Initialize (Self : in out Guard);
            overriding procedure Adjust (Self : in out Guard);
            overriding procedure Finalize (Self : in out Guard);
            type Root is tagged record
                Text : String (1 .. Count) := (others => 'a');
                Value : Integer := 8;
                Resource : Guard;
            end record;
            function Read (Self : Root) return Integer;
            type Child is new Root with record
                Extra : Integer := 13;
            end record;
            overriding function Read (Self : Child) return Integer;
        end Model;
        package body Model is
            overriding procedure Initialize (Self : in out Guard) is
            begin
                Live := Live + 1;
            end Initialize;
            overriding procedure Adjust (Self : in out Guard) is
            begin
                Live := Live + 1;
            end Adjust;
            overriding procedure Finalize (Self : in out Guard) is
            begin
                Live := Live - 1;
            end Finalize;
            function Read (Self : Root) return Integer is
            begin
                return Self.Text'Length + Self.Value;
            end Read;
            overriding function Read (Self : Child) return Integer is
            begin
                return Self.Text'Length + Self.Value + Self.Extra;
            end Read;
        end Model;
        Item : Model.Child;
        Wide : Model.Root'Class := Item;
        Copy : Model.Root'Class := Wide;
        type Link is access Model.Root'Class;
        Pointer : Link := new Model.Child;
        procedure Free is new Ada.Unchecked_Deallocation (Model.Root'Class, Link);
    begin
        Model.Child (Wide).Text (Count) := 'z';
        if Item.Text (Count) /= 'a' or else Model.Child (Copy).Text (Count) /= 'a'
            or else Model.Read (Wide) /= Count + 21
            or else Model.Read (Pointer.all) /= Count + 21 then
            raise Program_Error;
        end if;
        Copy := Wide;
        if Model.Child (Copy).Text (Count) /= 'z' then
            raise Program_Error;
        end if;
        if Live /= 4 then
            raise Program_Error;
        end if;
        Free (Pointer);
        if Live /= 3 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (3);
    exception
        when others => Ada.Text_IO.Put_Line ("tagged runtime worker failed");
    end Worker;
begin
    Run (7);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged runtime layouts passed");
end TaggedRuntime;
