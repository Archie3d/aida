with Ada.Text_IO; use Ada.Text_IO;
procedure AnonymousDispatch is
    package Shapes is
        type Root is tagged record
            Value : Integer := 1;
        end record;
        function Read (Item : access Root) return Integer;
        type Child is new Root with record
            Extra : Integer := 10;
        end record;
        overriding function Read (Item : access Child) return Integer;
    end Shapes;
    package body Shapes is
        function Read (Item : access Root) return Integer is
        begin
            return Item.Value;
        end Read;
        function Read (Item : access Child) return Integer is
        begin
            return Item.Value + Item.Extra;
        end Read;
    end Shapes;
    use Shapes;
    A : aliased Root;
    B : aliased Child;
    type Wide is access all Root'Class;
    P : Wide := B'Access;
    Calls : Integer := 0;
    function Selected return Wide is
    begin
        Calls := Calls + 1;
        return P;
    end Selected;
begin
    if Read (A'Access) /= 1 or Read (B'Access) /= 11 then raise Program_Error; end if;
    if Read (Selected) /= 11 or Calls /= 1 then raise Program_Error; end if;
    P := null;
    begin
        Calls := Read (P);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("anonymous controlling access passed");
end AnonymousDispatch;
