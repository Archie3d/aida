with Ada.Text_IO;
with Ada.Finalization;
with Ada.Unchecked_Deallocation;
procedure InterfaceOwnership is
    procedure Run (Count : Positive) is
        Live : Integer := 0;
        package Model is
            type Readable is interface;
            function Read (Self : Readable) return Integer is abstract;
            type Base is new Ada.Finalization.Controlled with null record;
            overriding procedure Initialize (Self : in out Base);
            overriding procedure Adjust (Self : in out Base);
            overriding procedure Finalize (Self : in out Base);
            type Hidden is new Base and Readable with private;
            overriding function Read (Self : Hidden) return Integer;
        private
            type Hidden is new Base and Readable with record
                Text : String (1 .. Count) := (others => 'x');
            end record;
        end Model;
        package body Model is
            overriding procedure Initialize (Self : in out Base) is
            begin
                Live := Live + 1;
            end Initialize;
            overriding procedure Adjust (Self : in out Base) is
            begin
                Live := Live + 1;
            end Adjust;
            overriding procedure Finalize (Self : in out Base) is
            begin
                Live := Live - 1;
            end Finalize;
            overriding function Read (Self : Hidden) return Integer is
            begin
                return Self.Text'Length;
            end Read;
        end Model;
        package Extensions is
            type Child is new Model.Hidden with null record;
        end Extensions;
    begin
        declare
            Object : Extensions.Child;
            R : Model.Readable'Class := Object;
            Copy : Model.Readable'Class := R;
            type Link is access Model.Readable'Class;
            Pointer : Link := new Extensions.Child;
            procedure Free is new Ada.Unchecked_Deallocation (Model.Readable'Class, Link);
        begin
            for Index in 1 .. 10 loop
                Copy := R;
                if Model.Read (Copy) /= Count or else Model.Read (Pointer.all) /= Count or else Live /= 4 then
                    raise Program_Error;
                end if;
            end loop;
            Free (Pointer);
        end;
        if Live /= 0 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run (17);
        Run (2);
    exception
        when others => Ada.Text_IO.Put_Line ("interface ownership worker failed");
    end Worker;
begin
    Run (11);
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("interface ownership passed");
end InterfaceOwnership;
