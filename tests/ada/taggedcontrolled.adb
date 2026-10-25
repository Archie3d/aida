with Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure TaggedControlled is
    procedure Run (Count : Positive) is
        Live : Integer := 0;
        package Model is
            type Root (Length : Positive) is new Ada.Finalization.Controlled with record
                Text : String (1 .. Length) := (others => 'a');
            end record;
            overriding procedure Initialize (Self : in out Root);
            overriding procedure Adjust (Self : in out Root);
            overriding procedure Finalize (Self : in out Root);
            type Child is new Root with record
                Extra : String (1 .. Length) := (others => 'b');
            end record;
        end Model;
        package body Model is
            overriding procedure Initialize (Self : in out Root) is
            begin
                Live := Live + 1;
            end Initialize;
            overriding procedure Adjust (Self : in out Root) is
            begin
                Live := Live + 1;
            end Adjust;
            overriding procedure Finalize (Self : in out Root) is
            begin
                Live := Live - 1;
            end Finalize;
        end Model;
    begin
        declare
            Item : Model.Child (Count);
            Other : Model.Child (Count);
            Wide : Model.Root'Class := Item;
            Copy : Model.Root'Class := Wide;
            type Link is access Model.Root'Class;
            Pointer : Link := new Model.Child (Count);
            procedure Free is new Ada.Unchecked_Deallocation (Model.Root'Class, Link);
        begin
            for Iteration in 1 .. 10 loop
                Model.Root (Item) := Model.Root (Other);
                Item.Extra (Count) := 'x';
                Model.Child (Wide).Text (Count) := 'z';
                if Model.Child (Copy).Text (Count) /= 'a' then
                    raise Program_Error;
                end if;
                Copy := Wide;
                if Model.Child (Copy).Text (Count) /= 'z' or else Other.Extra (Count) /= 'b' then
                    raise Program_Error;
                end if;
                Model.Child (Copy).Text (Count) := 'a';
            end loop;
            if Live /= 5 then
                raise Program_Error;
            end if;
            Free (Pointer);
        end;
        if Live /= 0 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (13);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("tagged controlled worker failed");
    end Worker;
begin
    Run (7);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged controlled storage passed");
end TaggedControlled;
