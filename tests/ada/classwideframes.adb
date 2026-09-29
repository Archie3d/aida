with Ada.Text_IO; use Ada.Text_IO;
with Ada.Tags; use Ada.Tags;
with Dispatch_Model; use Dispatch_Model;
procedure ClasswideFrames is
    type Link is access Root'Class;
    Pointer : Link;
    Previous : Tag := No_Tag;
    procedure Run (N : Integer) is
        Offset : Integer := N;
        type Local is new Root with record
            Y : Integer := 10;
        end record;
        overriding function Value (Item : Local) return Integer is
        begin
            return Item.X + Item.Y + Offset;
        end Value;
        A : Local;
        B : Root'Class := A;
        procedure Inner is
        begin
            Put_Line (Integer'Image (Apply (B)));
        end Inner;
    begin
        if Previous = Local'Tag then
            raise Program_Error;
        end if;
        Previous := Local'Tag;
        Inner;
        if N = 1 then
            Run (2);
            Inner;
        end if;
        begin
            Pointer := new Local;
            raise Constraint_Error;
        exception
            when Program_Error => Put_Line ("allocation accessibility");
        end;
    end Run;
    function Escape return Root'Class is
        type Local is new Root with null record;
        Item : Local;
    begin
        return Item;
    end Escape;
begin
    Run (1);
    Run (3);
    begin
        declare
            Bad : Root'Class := Escape;
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => Put_Line ("result accessibility");
    end;
    declare
        type Block_Local is new Root with null record;
    begin
        Pointer := new Block_Local;
        raise Constraint_Error;
    exception
        when Program_Error => Put_Line ("block accessibility");
    end;
end ClasswideFrames;
